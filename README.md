# CaRpg

CaRpg is a 3d role playing game somewhat based on Gothic series. Can be played in singleplayer or co-op multiplayer mode. 

![GitHub release](https://img.shields.io/github/release/Tomash667/carpg.svg) ![GitHub](https://img.shields.io/github/license/Tomash667/carpg.svg) ![GitHub issues by-label](https://img.shields.io/github/issues-raw/Tomash667/carpg/bug.svg?color=red&label=bugs)

## Docs

- [Readme ENG](https://github.com/Tomash667/carpg/blob/master/doc/readme_eng.txt)
- [Readme PL](https://github.com/Tomash667/carpg/blob/master/doc/readme.txt)
- [Changelog ENG](https://github.com/Tomash667/carpg/blob/master/doc/changelog_eng.txt)
- [Changelog PL](https://github.com/Tomash667/carpg/blob/master/doc/changelog.txt)

## How to build

Copy game content from latest release and put it inside bin/data folder.

CaRpg uses git submodule to link to [carpglib](https://github.com/Tomash667/carpglib) engine. To download everything use git command:
```
git submodule update --init
```

Open solution carpg.sln with Visual Studio 2017 (15.9.X) and use build. All dependencies and game should compile (client_dbg/client_dbg2 are for debugging multiplayer and can be unloaded).

Important branches:
- **dev** - development branch where all new features are added
- **master** - stable and tested branch where only fixes *should* be added

## Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change. 

For details look [here](https://github.com/Tomash667/carpg/blob/dev/CONTRIBUTING.md).

