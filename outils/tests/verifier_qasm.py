#!/usr/bin/env python3
"""verifier_qasm.py - verifie que l'OpenQASM exporte par la console donne, dans
Qiskit (la bibliotheque d'IBM), exactement les memes probabilites que le
simulateur de la carte. Les mesures sont retirees pour calculer le vecteur d'etat.

  pip install qiskit
  python3 verifier_qasm.py <dossier produit par exporter_qasm>
"""
import glob, os, sys
from qiskit import qasm2
from qiskit.quantum_info import Statevector

d = sys.argv[1] if len(sys.argv) > 1 else "."
ok = True
for f in sorted(glob.glob(os.path.join(d, "algo_*.qasm"))):
    qc = qasm2.load(f, custom_instructions=qasm2.LEGACY_CUSTOM_INSTRUCTIONS).remove_final_measurements(inplace=False)
    lignes = open(f[:-5] + ".txt").read().split("\n")
    nom, pc = lignes[0], [float(x) for x in lignes[1:] if x]
    pq = Statevector.from_instruction(qc).probabilities()
    ecart = max(abs(a - b) for a, b in zip(pc, pq))
    bon = ecart < 1e-4
    ok &= bon
    print(f"{'OK ' if bon else 'ERR'} {nom:36s} écart max {ecart:.1e}")
print("Tout concorde avec Qiskit." if ok else "DIFFÉRENCES !")
sys.exit(0 if ok else 1)
