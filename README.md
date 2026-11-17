# NewKarterz (Unreal Engine 5.8)

Jeu de karts arcade. Version Windows principale, sous Unreal Engine 5.8 (C++).

## Structure
- `Source/NewKarterz` : code C++ du jeu
  - `NKTrack` : construction des circuits (route, terrain, vibreurs, barrières, eau, décor instancié)
  - `NKKart` : pilotage arcade (accélération, saut, dérapage + turbos, hors-piste, IA)
  - `NKGameMode` : course, classement, éclairage (Lumen, ciel, nuages, brouillard), HUD
- `Content/NKData` : tracés des circuits, positions du décor, liste des personnages (JSON)
- `Content/NK` : modèles, matériaux, textures (Git LFS)
- `Scripts` : scripts Python d'import et de création de matériaux dans l'éditeur

## Compiler
`Build.bat NewKarterzEditor Win64 Development -Project=NewKarterz.uproject`

## Empaqueter
`RunUAT.bat BuildCookRun -project=NewKarterz.uproject -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -archive`
