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
1. Menus et objets : faits (v1). À enrichir : vrais modèles d'objets (boîte « ? » translucide, carapaces, banane),
   sons, effets d'impact, IA qui évite les bananes ; menus plus beaux (UMG/Slate, polices, vignettes des circuits),
   options (nombre de tours, difficulté, nombre de karts), écran de chargement.
2. Remplacer le décor low-poly par des modèles photoscannés CC0 (Poly Haven / ambientCG) en Nanite ; couleurs par instance.
3. Personnages animés (squelettes Melannie/Rocco), roues qui tournent, effets Niagara (poussière, étincelles, turbo).
4. Sons et musique (MetaSounds), commentateur.
5. Modes (Grand Prix, contre-la-montre, élimination, 2 joueurs) comme dans la version web.

## Journal / pièges (décor photoscanné)
- Sapins Poly Haven (`fir_tree_01`, CC0) : le glTF contient 3 arbres (a/b/c) côte à côte ; on n'exporte que `b_LOD0`
  sans décimation (`Scripts/blender_join_fir.py`) → `/Game/NK/PH/ph_fir2` (Nanite, 2,3 M triangles).
- Aiguilles invisibles en jeu : Nanite les élimine au loin → `shape_preservation = PRESERVE_AREA` (`nanite_area.py`).
- Matériaux feuillage : parent Interchange « Blend » (translucide) remplacé par `/Game/NK/Mat/M_Foliage`
  (masqué, deux faces, Two Sided Foliage, paramètre Tint) via `foliage_mat.py`.
- Remplacements par circuit : `NKData/tN_repl.json` (`{"i1":{"mesh":"ph_fir2","k":1.0}}`) ; `insp_decor.py` liste les groupes iN.

## Objets (NKItems.h/.cpp)
- `ANKItems` (créé par le GameMode) : boîtes = `Track->Boxes` + 2 rangées ajoutées à 1/3 et 2/3 du tour ; rendu par ISM
  reconstruits à chaque image (BasicShapes + `BasicShapeMaterial`, paramètre `Color`).
- Objets : banane, carapace verte (rebondit sur les bords), rouge (vise le kart devant), turbo, bouclier (8 s, 3 orbes).
  Tirage selon le classement (`Roll`). Kart : `Item`, `RollT` (roulette 1,1 s), `ShieldT`, `HitFlash`.
- Joueur : E / Ctrl / LB / B. L'IA utilise ses objets après 0,8–3,5 s. `nkdbg.txt` journalise objets/touches/boîtes.
- Caméra : pas de collision (une collision sur l'arche de départ plaquait la caméra dans le kart). Bras abaissé
  (720 cm, décalage Z 215, tangage −8°) pour passer sous l'arche.
- Le .exe empaqueté ignore `?track=N` sur la ligne de commande (démarre sur le circuit 0) — utiliser les menus.

## Menus (NKGameMode, ANKHUD sur Canvas)
- États : `Menu` (pages 0 titre, 1 pilote, 2 circuit) → `Countdown` → `Race` → `Finished` ; `bPaused` (pause).
- Options d'URL : `track=N?char=C?menu=M` (M=0 course directe, 1 titre, 3 choix du circuit). `-nkauto` saute les menus.
- Caméra de menu `MenuCam` en orbite ; choix du pilote = `Player->SetModel` en direct ; `StartRace()` échange le
  modèle avec l'adversaire qui avait le même pilote.
- Test : `-nkmenu` → captures `nk_menu_0_titre … 5_resultats.png` puis quitte (ne pas combiner avec -nkauto).

## Refonte graphique v2 (17/11)
- ⚠ Piège majeur : les matériaux importés (Interchange glTF) héritaient de parents du plugin sans l'usage
  « Instanced Static Meshes » → en jeu, matériau par défaut (décor gris/noir). `Scripts/fix_usage.py` copie les parents
  dans `/Game/NK/Mat/Gltf` avec les usages (ISM, Nanite) et rebranche toutes les instances (1749). À relancer après
  tout nouvel import ; vérifier `Saved/Logs/NewKarterz.log` (« missing usage flag »).
- Matériaux « Sheen » (fourrure des personnages) rendus noirs → `Scripts/fix_sheen.py` les rebranche sur MI_Default_Opaque.
- Couleurs par instance du décor : `Tools/texpcol.js` (puppeteer sur la version web, `test/build.sh`) → `NKData/tN_col.json`
  (alignées sur tN_inst.json). `M_DecorInst` = Base × couleur de sommet × PerInstanceCustomData(0..2) × aléa ;
  appliqué aux groupes sans texture (`Material_N`). Petites touffes teintées selon le biome dans `BuildDecor`.
- `M_Terrain2` (`Scripts/graphics_v2.py`) : chaque couche échantillonnée à 2 échelles selon la distance (anti-répétition),
  zones de mélange larges, variation macro ; les MI_Terrain_* y sont rebranchées.
- Éclairage : Lumen qualité 2, fuite de ciel 0,22, soleil doux (angle 1,2°), étalonnage (contraste, saturation, gain chaud),
  AO, netteté `r.Tonemapper.Sharpen=0.8`.
- Restes : palmiers/sapins web encore low-poly à certains endroits, corps de certains persos très sombres (couleurs
  d'origine ?), pas encore d'herbe dense ni de vent.

## Végétation, vent, effets (17/11 soir)
- Végétation Poly Haven : `Tools`→ PC `tools\ph_fetch.py` (Blender -b : télécharge le glTF 2k, exporte chaque variante
  `ue_import\ph\ph_<id>_<k>.glb`, centrée, base à z=0). Import : `Scripts/import_ph2.py` (Nanite PRESERVE_AREA,
  matériaux → M_Foliage + paramètre Wind) puis `fix_usage.py`.
  Modèles : grass_medium_02, fern_02, flower_gazania, wild_rooibos_bush (tailles réelles 10–90 cm → échelle ×3–10).
- Dispersion : `NKData/tN_scatter.json` → `ANKTrack::BuildScatter` (hauteur lue dans la grille du terrain, pas de trace :
  la collision est cuite en asynchrone ; exclut piste, raccourcis, eau, pentes). ~16–25 k instances / circuit.
- Vent : WPO dans M_Foliage (Wind=6 par défaut) et M_DecorInst (Wind=0, 5 pour les touffes) : sin(temps + position) × hauteur locale.
- Effets : `NKFx.h/.cpp` (ANKFx) — particules maison sur ISM + PerInstanceCustomData (rgb, alpha) :
  étincelles de dérapage (couleur du niveau de turbo), poussière hors-piste/dérapage (couleur du biome), flammes de turbo,
  anneau de poussière à l'atterrissage, étoiles quand un kart est touché. Matériaux `M_FxAdd` / `M_FxDust` (`fx_mats.py`).
- Objets : boîte en verre irisé `M_ItemBox` + cœur lumineux `M_ItemCore`, carapaces vernies avec anneau blanc,
  banane en V, orbes de bouclier lumineux (`item_mats.py`).
- Ambiances (ANKFx, autour de la caméra) : neige (Pic Givré, ~2200 flocons), lucioles clignotantes (Forêt), braises (Volcan).
- Forêt en lumière de fin d'après-midi (soleil −17°, teinte dorée, brouillard volumétrique chaud).
- À faire : quelques éléments du décor fixe très sombres (barrières, drapeaux à contre-jour), boîtes « ? » de la version web
  qui doublent les nôtres sur certains circuits, roues qui tournent, personnages animés, buissons tropicaux sur la neige.
