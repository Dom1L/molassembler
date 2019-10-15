import pytest

import molassembler
import os


def test_LineNotation():
    if not molassembler.io.LineNotation.enabled:
        return

    smiles = "CN1C=NC2=C1C(=O)N(C(=O)N2C)C"
    inchi = "InChI=1S/C8H10N4O2/c1-10-4-9-6-5(10)7(13)12(3)8(14)11(6)2/h4H,1-3H3"
    caffeine_from_smiles = molassembler.io.LineNotation.from_isomeric_smiles(
        smiles)
    assert caffeine_from_smiles.graph.N() == 24

    caffeine_from_inchi = molassembler.io.LineNotation.from_inchi(inchi)
    assert caffeine_from_inchi.graph.N() == 24


def test_FileIO():
    sample_MOL = """
 OpenBabel02081817133D

 50 54  0  0  0  0  0  0  0  0999 V2000
   -4.4926   -0.0963    0.8478 Fe  0  0  0  0  0  0  0  0  0  0  0  0
   -2.0547   -0.1571   -1.7896 Fe  0  0  0  0  0  0  0  0  0  0  0  0
   -1.8137   -1.8755   -1.4592 C   0  0  0  0  0  0  0  0  0  0  0  0
   -4.9690    2.9993    0.9255 C   0  0  0  0  0  0  0  0  0  0  0  0
   -6.2495   -0.2575    0.8047 C   0  0  0  0  0  0  0  0  0  0  0  0
   -4.4087    0.0418    2.8405 C   0  0  0  0  0  0  0  0  0  0  0  0
   -4.2943   -1.8440    1.0091 C   0  0  0  0  0  0  0  0  0  0  0  0
   -2.1073   -0.5913   -3.7407 C   0  0  0  0  0  0  0  0  0  0  0  0
   -0.3133    0.1302   -1.7772 C   0  0  0  0  0  0  0  0  0  0  0  0
   -4.1421    1.4106   -2.1614 C   0  0  0  0  0  0  0  0  0  0  0  0
   -4.4087   -0.2385   -1.6000 S   0  0  2  0  0  0  0  0  0  0  0  0
   -2.1833    0.3528    0.6084 S   0  0  1  0  0  0  0  0  0  0  0  0
   -1.6646   -3.0092   -1.2742 O   0  0  0  0  0  0  0  0  0  0  0  0
   -4.1552   -2.9856    1.1480 O   0  0  0  0  0  0  0  0  0  0  0  0
    0.8357    0.2620   -1.6984 O   0  0  0  0  0  0  0  0  0  0  0  0
   -7.3920   -0.4341    0.7199 O   0  0  0  0  0  0  0  0  0  0  0  0
   -3.1054   -0.3642   -4.4029 O   0  0  0  0  0  0  0  0  0  0  0  0
   -0.8698   -1.1686   -4.4134 C   0  0  0  0  0  0  0  0  0  0  0  0
   -0.1336   -0.3618   -4.5173 H   0  0  0  0  0  0  0  0  0  0  0  0
   -1.1162   -1.5593   -5.4055 H   0  0  0  0  0  0  0  0  0  0  0  0
   -0.4134   -1.9460   -3.7945 H   0  0  0  0  0  0  0  0  0  0  0  0
   -3.5018    0.6459    3.3865 O   0  0  0  0  0  0  0  0  0  0  0  0
   -5.5178   -0.5780    3.6781 C   0  0  0  0  0  0  0  0  0  0  0  0
   -5.7589   -1.5831    3.3211 H   0  0  0  0  0  0  0  0  0  0  0  0
   -6.4197    0.0348    3.5564 H   0  0  0  0  0  0  0  0  0  0  0  0
   -5.2378   -0.5998    4.7359 H   0  0  0  0  0  0  0  0  0  0  0  0
   -4.2118    1.8820    0.8186 N   0  0  0  0  0  0  0  0  0  0  0  0
   -2.8654    1.9744    0.7142 C   0  0  0  0  0  0  0  0  0  0  0  0
   -2.1940    3.1913    0.7001 C   0  0  0  0  0  0  0  0  0  0  0  0
   -2.9612    4.3485    0.7926 C   0  0  0  0  0  0  0  0  0  0  0  0
   -4.3490    4.2494    0.9058 C   0  0  0  0  0  0  0  0  0  0  0  0
   -6.4516    2.8426    1.0585 C   0  0  0  0  0  0  0  0  0  0  0  0
   -6.6946    2.2270    1.9313 H   0  0  0  0  0  0  0  0  0  0  0  0
   -6.8673    2.3465    0.1751 H   0  0  0  0  0  0  0  0  0  0  0  0
   -6.9334    3.8154    1.1698 H   0  0  0  0  0  0  0  0  0  0  0  0
   -1.1134    3.2193    0.6168 H   0  0  0  0  0  0  0  0  0  0  0  0
   -2.4832    5.3234    0.7784 H   0  0  0  0  0  0  0  0  0  0  0  0
   -4.9615    5.1413    0.9820 H   0  0  0  0  0  0  0  0  0  0  0  0
   -2.8115    1.6264   -2.2824 N   0  0  0  0  0  0  0  0  0  0  0  0
   -2.3483    2.8228   -2.7145 C   0  0  0  0  0  0  0  0  0  0  0  0
   -3.2564    3.8398   -3.0132 C   0  0  0  0  0  0  0  0  0  0  0  0
   -4.6292    3.6223   -2.8840 C   0  0  0  0  0  0  0  0  0  0  0  0
   -5.0919    2.3821   -2.4549 C   0  0  0  0  0  0  0  0  0  0  0  0
   -0.8688    3.0038   -2.8520 C   0  0  0  0  0  0  0  0  0  0  0  0
   -0.6364    4.0016   -3.2284 H   0  0  0  0  0  0  0  0  0  0  0  0
   -0.4550    2.2612   -3.5426 H   0  0  0  0  0  0  0  0  0  0  0  0
   -0.3733    2.8717   -1.8844 H   0  0  0  0  0  0  0  0  0  0  0  0
   -2.8793    4.7997   -3.3489 H   0  0  0  0  0  0  0  0  0  0  0  0
   -5.3320    4.4159   -3.1185 H   0  0  0  0  0  0  0  0  0  0  0  0
   -6.1487    2.1651   -2.3464 H   0  0  0  0  0  0  0  0  0  0  0  0
  1  7  1  0  0  0  0
  1  6  1  0  0  0  0
  2  9  1  0  0  0  0
  2 11  1  0  0  0  0
  2  3  1  0  0  0  0
  3 13  1  0  0  0  0
  4 32  1  0  0  0  0
  5  1  1  0  0  0  0
  6 22  2  0  0  0  0
  6 23  1  0  0  0  0
  7 14  1  0  0  0  0
  8  2  1  0  0  0  0
  9 15  1  0  0  0  0
 11 10  1  6  0  0  0
 11  1  1  0  0  0  0
 12  2  1  6  0  0  0
 12 28  1  0  0  0  0
 12  1  1  0  0  0  0
 16  5  1  0  0  0  0
 17  8  2  0  0  0  0
 18 21  1  0  0  0  0
 18  8  1  0  0  0  0
 19 18  1  0  0  0  0
 20 18  1  0  0  0  0
 23 26  1  0  0  0  0
 24 23  1  0  0  0  0
 25 23  1  0  0  0  0
 27  1  1  0  0  0  0
 27  4  1  0  0  0  0
 28 27  1  0  0  0  0
 29 28  2  0  0  0  0
 29 30  1  0  0  0  0
 30 31  1  0  0  0  0
 31  4  2  0  0  0  0
 31 38  1  0  0  0  0
 32 35  1  0  0  0  0
 32 33  1  0  0  0  0
 34 32  1  0  0  0  0
 36 29  1  0  0  0  0
 37 30  1  0  0  0  0
 39 10  1  0  0  0  0
 39  2  1  0  0  0  0
 40 39  1  0  0  0  0
 41 42  1  0  0  0  0
 41 40  2  0  0  0  0
 42 43  1  0  0  0  0
 43 50  1  0  0  0  0
 43 10  2  0  0  0  0
 44 40  1  0  0  0  0
 44 47  1  0  0  0  0
 45 44  1  0  0  0  0
 46 44  1  0  0  0  0
 48 41  1  0  0  0  0
 49 42  1  0  0  0  0
M  END"""

    with open("sample.mol", "w") as mol_file:
        mol_file.write(sample_MOL)

    mol = molassembler.io.read("sample.mol")

    # Interpret multiple from mol, expecting only a single one
    splat = molassembler.io.split("sample.mol")
    assert len(splat) == 1

    assert mol.graph.N() == 50

    # Clean up
    os.remove("sample.mol")
