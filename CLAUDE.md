# NewKarterz — notes de travail pour Claude

Jeu de karts arcade d'Antoine (tatane999). Version principale : **Windows, Unreal Engine 5.8, C++**.
L'ancienne version web (Three.js) sert de référence de gameplay ; elle n'est plus développée.

## Où sont les choses
- PC d'Antoine : `C:\Users\Antoine\Documents\NewKarterz_3D\` (seul dossier connecté)
  - `NewKarterzUE\` : ce dépôt (projet Unreal)
  - `Build\Windows\NewKarterz.exe` : dernière version empaquetée (raccourci sur le bureau)
  - `ue_import\` : sources glTF (karts, personnages, décor) ; `ue_import\joined\` = versions fusionnées importées dans UE
  - `pbr2k\` : textures Poly Haven 2K (CC0)
  - `tools\git\` : Git portable (MinGit + git-lfs), `tools\git\cmd\git.exe`
- Unreal : `C:\Program Files\Epic Games\UE_5.8\` ; MSVC Build Tools 2022 installés.

## Piloter le PC
- La passerelle principale est le serveur MCP de **Blender** (`execute_blender_code`) : Python avec `subprocess` pour lancer
  Build.bat, UnrealEditor(-Cmd).exe, RunUAT, git. Si Blender est fermé, l'ouvrir via computer-use (« Blender 5.3 »).
- Les appels Blender expirent vers 80 s : lancer les tâches longues avec `subprocess.Popen` et vérifier ensuite.
- `device_commit_files` met en cache par chemin de staging : toujours utiliser un **nouveau dossier** sous
  `/mnt/user-data/outputs/pN/` pour chaque envoi, sinon une ancienne version du fichier est écrite.
- Edge n'est accessible qu'en lecture (computer-use) ; utiliser le navigateur intégré pour le web interactif.

## Boucle de travail
1. Modifier le C++ (`Source/NewKarterz`) ou les scripts (`Scripts/*.py`).
2. Compiler : `Engine\Build\BatchFiles\Build.bat NewKarterzEditor Win64 Development -Project=...\NewKarterz.uproject -WaitMutex -NoHotReload`
   (fermer tout éditeur Unreal ouvert : Live Coding bloque la compilation).
3. Tester sans fenêtre de jeu interactive : `UnrealEditor.exe NewKarterz.uproject /Game/Maps/Race?track=N -game -windowed -nkauto -nkshots`
   → le joueur est piloté par l'IA, 5 captures dans `Saved\Screenshots\WindowsEditor\nk_tN_*.png`, journal dans `Saved\nkdbg.txt`, puis le jeu quitte.
4. Scripts éditeur (import, matériaux) : `UnrealEditor-Cmd.exe NKAssets.uproject -run=pythonscript -script=...`
   (`NKAssets.uproject` = copie sans module C++, pour lancer Python même si le C++ ne compile pas).
5. Empaqueter : `RunUAT.bat BuildCookRun -project=... -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -archive -archivedirectory=...\Build -unattended -nocompileeditor`
   Les assets chargés par chemin texte sont cuits grâce à `DirectoriesToAlwaysCook=/Game/NK` (DefaultGame.ini).
   Toujours vérifier une capture du **.exe empaqueté** (`%LOCALAPPDATA%\NewKarterz\Saved\Screenshots\Windows`) avant de l'annoncer.
6. Versionner : commit avec le git portable sur le PC ; pousser sur GitHub `tatane999/NewKarterz` (LFS pour uasset/umap/glb/png/jpg).

## Conventions
- Unités : 1 m du jeu web = 100 cm UE ; conversion JS (x,y,z) → UE (x·100, z·100, y·100) — identique à l'import glTF d'Unreal.
- Circuits : `Content/NKData/trackN.json` (ligne centrale, normales, limites, raccourcis, terrain), décor : `tN_inst.json` + `/Game/NK/Decor/tN_*`.
- Personnages : `/Game/NK/Chars/cN` (kart + pilote fusionnés), noms dans `NKData/chars.json`.
- Textes de l'interface et messages à Antoine : en **français**.

## Pistes d'amélioration (par priorité)
1. Objets (boîtes, carapaces, turbo, bouclier…) et menus (choix perso/kart/circuit, pause, résultats).
2. Remplacer le décor low-poly par des modèles photoscannés CC0 (Poly Haven / ambientCG) en Nanite ; couleurs par instance.
3. Personnages animés (squelettes Melannie/Rocco), roues qui tournent, effets Niagara (poussière, étincelles, turbo).
4. Sons et musique (MetaSounds), commentateur.
5. Modes (Grand Prix, contre-la-montre, élimination, 2 joueurs) comme dans la version web.
