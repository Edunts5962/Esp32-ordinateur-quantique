// exporter_qasm.cpp - ecrit l'OpenQASM de chaque algorithme de la console et les
// probabilites calculees par son simulateur (pour comparer avec Qiskit).
//   g++ -std=c++17 -I../../firmware/quantique_console exporter_qasm.cpp ../../firmware/quantique_console/quantique.cpp -o exporter_qasm
//   ./exporter_qasm dossier && python3 verifier_qasm.py dossier
#include "quantique.h"
#include <cstdio>
#include <string>
int main(int argc, char **argv)
{
    std::string d = argc > 1 ? argv[1] : ".";
    for (int i = 0; i < q::NB_ALGOS; i++) {
        q::Circuit c;
        q::charger_algo(c, i);
        char qasm[2048];
        q::vers_qasm(c, qasm, sizeof qasm);
        char nom[256];
        snprintf(nom, sizeof nom, "%s/algo_%02d.qasm", d.c_str(), i);
        FILE *f = fopen(nom, "w"); fputs(qasm, f); fclose(f);
        q::Etat e; q::executer(c, e);
        float p[32]; q::probabilites(e, p);
        snprintf(nom, sizeof nom, "%s/algo_%02d.txt", d.c_str(), i);
        f = fopen(nom, "w");
        fprintf(f, "%s\n", q::ALGOS[i].nom);
        for (int k = 0; k < (1 << c.nq); k++) fprintf(f, "%.6f\n", p[k]);
        fclose(f);
    }
    // circuits au hasard : toutes les portes, avec ou sans controle, SWAP controle
    unsigned g = 7;
    auto r = [&](int n) { g = g * 1103515245u + 12345u; return (int)((g >> 16) % n); };
    for (int i = 0; i < 30; i++) {
        q::Circuit c;
        q::vider(c, 4);
        for (int col = 0; col < 10; col++) {
            int t = r(4), ctl = r(3) ? -1 : r(4);
            if (r(6) == 0) {                       // swap (eventuellement controle)
                int a = r(4), b = (a + 1 + r(3)) % 4;
                q::poser(c, col, a, q::P_SWAP); q::poser(c, col, b, q::P_SWAP);
                continue;
            }
            int p = q::P_H + r(11);
            q::poser(c, col, t, p, (r(100) - 50) / 15.0f);
            if (ctl >= 0 && ctl != t) q::poser(c, col, ctl, q::P_CTRL);
        }
        char qasm[2048];
        if (!q::vers_qasm(c, qasm, sizeof qasm)) continue;
        char nom[256];
        snprintf(nom, sizeof nom, "%s/algo_%02d.qasm", d.c_str(), 50 + i);
        FILE *f = fopen(nom, "w"); fputs(qasm, f); fclose(f);
        q::Etat e; q::executer(c, e);
        float p[32]; q::probabilites(e, p);
        snprintf(nom, sizeof nom, "%s/algo_%02d.txt", d.c_str(), 50 + i);
        f = fopen(nom, "w");
        fprintf(f, "circuit au hasard %d\n", i);
        for (int k = 0; k < 16; k++) fprintf(f, "%.6f\n", p[k]);
        fclose(f);
    }
    return 0;
}
