// =====================================================================
// quantique_console.ino - v1.0 "Ordinateur quantique" : 5 qubits dans la main
// Sur la carte ES3C28P (ESP32-S3, ecran 2,8" ILI9341 tactile FT6336, SD,
// codec audio ES8311 avec haut-parleur et micro integres).
//
//   CIRCUIT  : on pose des portes (H, X, Y, Z, S, T, Rx, Ry, Rz, CNOT, Toffoli,
//              SWAP, mesure) sur 1 a 5 qubits ; lecture pas a pas animee.
//   SPHERE   : la sphere de Bloch de chaque qubit, et un bac a sable.
//   MESURES  : des centaines de tirs (hasard materiel de l'ESP32), histogramme,
//              amplitudes, bruit d'un vrai QPU ; export vers un VRAI ordinateur
//              quantique (OpenQASM 2.0 en QR code pour IBM Quantum Composer).
//   ALGOS    : les algorithmes quantiques connus, a quoi ils servent, ou on en est.
//   TUTO     : les differents types d'equations du calcul quantique.
//
// Le calcul est EXACT : le vecteur d'etat (32 amplitudes complexes) est
// simule par la puce, comme le font les simulateurs des laboratoires.
//
// Architecture :
//   - coeur 0 : tache "son" (I2S 48 kHz : clics des mesures), tache "ecran"
//               (envoi de l'image) ;
//   - coeur 1 : loop() lit le tactile, calcule et dessine l'image suivante.
//
// Reglages Arduino IDE (identiques a la console NES et aux projets precedents) :
//   paquet esp32 3.x, carte "ESP32S3 Dev Module", PSRAM "OPI PSRAM",
//   Flash 16 Mo, partition "16M Flash (3MB APP/9.9MB FATFS)".
//   Bibliotheque TFT_eSPI deja configuree pour l'ecran (meme User_Setup.h).
// =====================================================================
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SD_MMC.h>
#include <WiFi.h>
#include <Preferences.h>
#include "esp_heap_caps.h"
#include "esp_random.h"
#include "driver/i2c_master.h"
#include "driver/i2s_std.h"
#include "ft6336_touch.h"
#include "gfx.h"
#include "interface.h"
#include "plateforme.h"

// ---------------- Broches ES3C28P ----------------------------------------
#define LCD_BL_PIN     45
#define I2C_SDA        16              // bus partage : tactile FT6336 + codec ES8311
#define I2C_SCL        15
#define TOUCH_RST      18
#define SD_SCK         38
#define SD_CMD         40
#define SD_D0          39
#define SD_D1          41
#define SD_D2          48
#define SD_D3          47
#define I2S_MCLK       4
#define I2S_BCLK       5
#define I2S_WS         7
#define I2S_DOUT       8               // ESP32 -> codec (haut-parleur)
#define I2S_DIN        6               // codec -> ESP32 (micro, inutilise ici)
#define PA_PIN         1               // ampli du haut-parleur
#define ES8311_ADR     0x18

#define FICHIER_CSV    "/quantique.csv"
#define DOSSIER_SD     "/quantique"
#define FS_SON         48000
#define CH             240             // echantillons par passage de la tache son (5 ms)

TFT_eSPI tft = TFT_eSPI();

// =========================================================================
// Plateforme (voir plateforme.h)
// =========================================================================
static bool s_sd_ok = false;
static Preferences s_prefs;
static bool s_son_ok = false;
static bool s_radio = false;
static char s_son_etat[40] = "démarrage…";

uint32_t plat_ms() { return millis(); }
void plat_luminosite(int pct) { ledcWrite(LCD_BL_PIN, (uint32_t)(pct * 255 / 100)); }
bool plat_sd() { return s_sd_ok; }
uint32_t plat_memoire_libre() { return heap_caps_get_free_size(MALLOC_CAP_INTERNAL); }
bool plat_son_ok() { return s_son_ok; }
const char *plat_son_etat() { return s_son_etat; }

// esp_random() est un VRAI generateur aleatoire quand la radio (Wi-Fi) est allumee :
// il melange le bruit thermique du recepteur radio. Sans radio, il est pseudo-aleatoire.
uint32_t plat_alea_materiel() { return esp_random(); }
const char *plat_alea_source() { return s_radio ? "bruit radio (vrai hasard)" : "pseudo-aléatoire"; }

void plat_csv(const char *ligne)
{
    if (!s_sd_ok) return;
    fs::File f = SD_MMC.open(FICHIER_CSV, FILE_APPEND);
    if (!f) { s_sd_ok = false; return; }
    f.println(ligne);
    f.close();
}

bool plat_ecrire_fichier(const char *nom, const char *texte)
{
    if (!s_sd_ok) return false;
    if (!SD_MMC.exists(DOSSIER_SD)) SD_MMC.mkdir(DOSSIER_SD);
    char chemin[96];
    snprintf(chemin, sizeof chemin, "%s/%s", DOSSIER_SD, nom);
    fs::File f = SD_MMC.open(chemin, FILE_WRITE);
    if (!f) return false;
    size_t n = f.print(texte);
    f.close();
    Serial.printf("[QU] Fichier ecrit : %s (%u octets)\n", chemin, (unsigned)n);
    return n > 0;
}

bool plat_nvs_lire(const char *cle, void *buf, size_t n)
{
    if (s_prefs.getBytesLength(cle) != n) return false;
    return s_prefs.getBytes(cle, buf, n) == n;
}

void plat_nvs_ecrire(const char *cle, const void *buf, size_t n) { s_prefs.putBytes(cle, buf, n); }

// =========================================================================
// Codec ES8311 (I2C) : esclave, MCLK = 256 x 48 kHz, I2S 16 bits
// (meme sequence que les consoles "vitesse du son" et "Fourier & Laplace")
// =========================================================================
static i2c_master_bus_handle_t s_bus = NULL;
static i2c_master_dev_handle_t s_codec = NULL;

static bool es_ecrire(uint8_t reg, uint8_t val)
{
    uint8_t b[2] = { reg, val };
    return i2c_master_transmit(s_codec, b, 2, 50) == ESP_OK;
}

static int es_lire(uint8_t reg)
{
    uint8_t v = 0;
    if (i2c_master_transmit_receive(s_codec, &reg, 1, &v, 1, 50) != ESP_OK) return -1;
    return v;
}

static bool init_codec()
{
    i2c_device_config_t cfg = {};
    cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    cfg.device_address = ES8311_ADR;
    cfg.scl_speed_hz = 100000;
    if (i2c_master_bus_add_device(s_bus, &cfg, &s_codec) != ESP_OK) return false;
    if (es_lire(0xFD) < 0) return false;
    es_ecrire(0x00, 0x1F); delay(20); es_ecrire(0x00, 0x00);
    es_ecrire(0x45, 0x00);
    es_ecrire(0x01, 0x30); es_ecrire(0x02, 0x00); es_ecrire(0x03, 0x10);
    es_ecrire(0x16, 0x24); es_ecrire(0x04, 0x10); es_ecrire(0x05, 0x00);
    es_ecrire(0x0B, 0x00); es_ecrire(0x0C, 0x00); es_ecrire(0x10, 0x1F); es_ecrire(0x11, 0x7F);
    es_ecrire(0x00, 0x80);
    es_ecrire(0x01, 0x3F);
    es_ecrire(0x02, 0x00); es_ecrire(0x05, 0x00);
    es_ecrire(0x03, 0x10); es_ecrire(0x04, 0x10);
    es_ecrire(0x07, 0x00); es_ecrire(0x08, 0xFF); es_ecrire(0x06, 0x03);
    es_ecrire(0x09, 0x0C); es_ecrire(0x0A, 0x0C);
    es_ecrire(0x13, 0x10);
    es_ecrire(0x1B, 0x0A); es_ecrire(0x1C, 0x6A);
    es_ecrire(0x17, 0xBF);
    es_ecrire(0x0E, 0x02);
    es_ecrire(0x12, 0x00);
    es_ecrire(0x0D, 0x01);
    es_ecrire(0x15, 0x40);
    es_ecrire(0x37, 0x08);
    es_ecrire(0x45, 0x00);
    es_ecrire(0x32, 0xB6);                         // volume du DAC
    return true;
}

// =========================================================================
// Son : une tache joue des "clics" (petits bips qui s'eteignent vite)
// =========================================================================
static i2s_chan_handle_t s_tx = NULL;
static volatile float b_f = 0;
static volatile int b_n = 0;                       // echantillons restants du bip demande
static volatile float b_vol = 0.3f;
static volatile bool b_nouveau = false;

static bool init_i2s()
{
    i2s_chan_config_t cc = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    cc.dma_desc_num = 4;
    cc.dma_frame_num = CH;
    cc.auto_clear = true;
    if (i2s_new_channel(&cc, &s_tx, NULL) != ESP_OK) return false;
    i2s_std_config_t sc = {};
    sc.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(FS_SON);
    sc.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
    sc.gpio_cfg.mclk = (gpio_num_t)I2S_MCLK;
    sc.gpio_cfg.bclk = (gpio_num_t)I2S_BCLK;
    sc.gpio_cfg.ws = (gpio_num_t)I2S_WS;
    sc.gpio_cfg.dout = (gpio_num_t)I2S_DOUT;
    sc.gpio_cfg.din = I2S_GPIO_UNUSED;
    if (i2s_channel_init_std_mode(s_tx, &sc) != ESP_OK) return false;
    i2s_channel_enable(s_tx);
    return true;
}

static void tache_son(void *arg)
{
    static int16_t sortie[2 * CH];
    float phase = 0, env = 0, f = 1000;
    int restant = 0;
    for (;;) {
        if (b_nouveau) { b_nouveau = false; f = b_f; restant = b_n; env = b_vol; phase = 0; }
        for (int i = 0; i < CH; i++) {
            int16_t s = 0;
            if (restant > 0) {
                restant--;
                env *= 0.9992f;                    // decroissance douce : un "tic" cristallin
                phase += 2 * PI * f / FS_SON;
                if (phase > 2 * PI) phase -= 2 * PI;
                s = (int16_t)(32767 * env * sinf(phase));
            }
            sortie[2 * i] = sortie[2 * i + 1] = s;
        }
        size_t nb = 0;
        i2s_channel_write(s_tx, sortie, sizeof sortie, &nb, portMAX_DELAY);
    }
}

void plat_bip(float f, int ms, int volume)
{
    if (!s_son_ok) return;
    static const float V[3] = { 0.12f, 0.25f, 0.5f };
    b_f = f;
    b_n = FS_SON * ms / 1000 * 3;                  // la queue de l'enveloppe prolonge un peu le son
    b_vol = V[volume < 0 ? 0 : (volume > 2 ? 2 : volume)];
    b_nouveau = true;
}

static void init_son()
{
    pinMode(PA_PIN, OUTPUT);
    digitalWrite(PA_PIN, HIGH);                    // ampli en marche
    if (!init_i2s()) { snprintf(s_son_etat, sizeof s_son_etat, "I2S en échec"); return; }
    if (!s_bus || !init_codec()) { snprintf(s_son_etat, sizeof s_son_etat, "codec ES8311 absent"); return; }
    xTaskCreatePinnedToCore(tache_son, "son", 4096, NULL, 5, NULL, 0);
    s_son_ok = true;
    snprintf(s_son_etat, sizeof s_son_etat, "clics à chaque mesure");
}

// =========================================================================
// Ecran : deux tampons, envoi sur le coeur 0
// =========================================================================
static uint16_t *s_tampon[2] = { nullptr, nullptr };
static uint16_t *s_fond = nullptr;
static bool s_double = false;
static int s_courant = 0;
static volatile int s_a_envoyer = 0;
static SemaphoreHandle_t s_sem_pret, s_sem_libre;

static void envoyer(const uint16_t *img) { tft.pushImage(0, 0, 320, 240, (uint16_t *)img); }

static void tache_ecran(void *arg)
{
    for (;;) {
        xSemaphoreTake(s_sem_pret, portMAX_DELAY);
        envoyer(s_tampon[s_a_envoyer]);
        xSemaphoreGive(s_sem_libre);
        vTaskDelay(1);
    }
}

static bool init_tampons()
{
    const size_t taille = 320 * 240 * sizeof(uint16_t);
    s_tampon[0] = (uint16_t *)heap_caps_malloc(taille, MALLOC_CAP_SPIRAM);
    s_tampon[1] = (uint16_t *)heap_caps_malloc(taille, MALLOC_CAP_SPIRAM);
    s_fond = (uint16_t *)heap_caps_malloc(taille, MALLOC_CAP_SPIRAM);
    if (s_tampon[0] && s_tampon[1]) {
        s_double = true;
    } else {
        Serial.println("[QU] PSRAM absente : un seul tampon, animations ralenties");
        if (s_tampon[0]) heap_caps_free(s_tampon[0]);
        if (s_tampon[1]) heap_caps_free(s_tampon[1]);
        if (s_fond) heap_caps_free(s_fond);
        s_fond = nullptr;
        s_tampon[1] = nullptr;
        s_tampon[0] = (uint16_t *)heap_caps_malloc(taille, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!s_tampon[0]) return false;
    }
    return true;
}

// =========================================================================
// Tactile (meme etalonnage que la console NES et les projets precedents)
// =========================================================================
static bool s_tactile_ok = false;

static bool init_bus()
{
    i2c_master_bus_config_t cfg = {};
    cfg.i2c_port = I2C_NUM_0;
    cfg.sda_io_num = (gpio_num_t)I2C_SDA;
    cfg.scl_io_num = (gpio_num_t)I2C_SCL;
    cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    cfg.glitch_ignore_cnt = 7;
    cfg.flags.enable_internal_pullup = true;
    return i2c_new_master_bus(&cfg, &s_bus) == ESP_OK;
}

static bool lire_appui_brut(int *rx, int *ry)
{
    static int xs[64], ys[64];
    while (ft6336_touch_get_raw(rx, ry)) delay(10);
    uint32_t t0 = millis();
    while (!ft6336_touch_get_raw(rx, ry)) {
        if (millis() - t0 > 30000) return false;
        delay(10);
    }
    int n = 0;
    while (ft6336_touch_get_raw(&xs[n % 64], &ys[n % 64])) { n++; delay(10); }
    if (n > 64) n = 64;
    if (n < 3) return false;
    qsort(xs, n, sizeof(int), [](const void *a, const void *b) { return *(int *)a - *(int *)b; });
    qsort(ys, n, sizeof(int), [](const void *a, const void *b) { return *(int *)a - *(int *)b; });
    *rx = xs[n / 2];
    *ry = ys[n / 2];
    return true;
}

static void etalonner()
{
    const int px[3] = { 30, 290, 160 }, py[3] = { 30, 120, 210 };
    int rx[3], ry[3];
    for (;;) {
        bool ok = true;
        for (int i = 0; i < 3 && ok; i++) {
            gfx::fb = s_tampon[0];
            gfx::degrade_v(0, 0, 320, 240, gfx::rgb(4, 4, 20), gfx::rgb(18, 6, 42));
            gfx::texte(160, 96, "Étalonnage de l'écran", POLICE_16, gfx::rgb(240, 242, 255), 255, gfx::CENTRE);
            gfx::texte(160, 120, "Touche le centre de la croix", POLICE_13, gfx::rgb(166, 170, 220), 255, gfx::CENTRE);
            gfx::halo(px[i], py[i], 18, gfx::rgb(40, 232, 255), 160);
            gfx::trait(px[i] - 12, py[i], px[i] + 12, py[i], 2, gfx::rgb(40, 232, 255));
            gfx::trait(px[i], py[i] - 12, px[i], py[i] + 12, 2, gfx::rgb(40, 232, 255));
            envoyer(s_tampon[0]);
            ok = lire_appui_brut(&rx[i], &ry[i]);
        }
        if (ok && ft6336_touch_set_calibration(rx[0], ry[0], px[0], py[0], rx[1], ry[1], px[1], py[1],
                                               rx[2], ry[2], px[2], py[2])) {
            ft6336_touch_save_calibration();
            return;
        }
    }
}

// =========================================================================
static void init_sd()
{
    if (!SD_MMC.setPins(SD_SCK, SD_CMD, SD_D0, SD_D1, SD_D2, SD_D3)) return;
    if (!SD_MMC.begin("/sdcard", true)) return;
    s_sd_ok = true;
    if (!SD_MMC.exists(FICHIER_CSV)) {
        fs::File f = SD_MMC.open(FICHIER_CSV, FILE_WRITE);
        if (f) {
            f.println("# circuit,tirs,bruit(0 parfait/1 faible/2 QPU),hasard,resultat:compte,...");
            f.close();
        }
    }
}

void setup()
{
    Serial.begin(115200);
#if ARDUINO_USB_CDC_ON_BOOT
    Serial.setTxTimeoutMs(0);                      // ne jamais bloquer si le moniteur est ferme
#endif
    delay(200);
    Serial.printf("\n[QU] Console Ordinateur quantique %s\n", OQ_VERSION);

    ledcAttachChannel(LCD_BL_PIN, 2000, 8, 0);
    ledcWrite(LCD_BL_PIN, 204);
    tft.init();
    tft.setRotation(1);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);

    s_prefs.begin("quantique", false);
    if (!init_tampons()) {
        tft.drawString("Memoire insuffisante", 10, 10, 2);
        for (;;) delay(1000);
    }
    gfx::init(s_tampon[0]);

    if (init_bus()) {
        s_tactile_ok = ft6336_touch_init(s_bus, (gpio_num_t)TOUCH_RST, 320, 240);
        if (!s_tactile_ok) Serial.println("[QU] Tactile introuvable");
        else if (!ft6336_touch_load_calibration()) etalonner();
    }

    // la radio allumee (sans se connecter a rien) : esp_random() devient un vrai hasard physique
    s_radio = WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    Serial.printf("[QU] Hasard : %s (exemple %08lX)\n", plat_alea_source(), (unsigned long)esp_random());

    init_sd();
    init_son();
    Serial.printf("[QU] Son : %s · SD : %s\n", s_son_etat, s_sd_ok ? "oui" : "non");

    ui::init(s_fond);

    if (s_double) {
        s_sem_pret = xSemaphoreCreateBinary();
        s_sem_libre = xSemaphoreCreateBinary();
        xSemaphoreGive(s_sem_libre);
        xTaskCreatePinnedToCore(tache_ecran, "ecran", 4096, NULL, 3, NULL, 0);
    }
    Serial.printf("[QU] Pret - PSRAM libre %u Ko, interne %u Ko\n",
                  (unsigned)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024),
                  (unsigned)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024));
}

void loop()
{
    int x = 0, y = 0;
    bool appui = s_tactile_ok && ft6336_touch_read(&x, &y);
    ui::toucher(appui, x, y);

    gfx::fb = s_tampon[s_courant];
    ui::image(millis());

    if (s_double) {
        xSemaphoreTake(s_sem_libre, portMAX_DELAY);
        s_a_envoyer = s_courant;
        xSemaphoreGive(s_sem_pret);
        s_courant ^= 1;
    } else {
        envoyer(s_tampon[0]);
    }
}
