# Deep Shelter 3D

Oryginalna gra zarządzania podziemnym schronem dla Nintendo 3DS, rozwijana jako aplikacja homebrew w C++ z użyciem devkitARM, libctru, Citro3D i Citro2D.

Projekt nie jest powiązany z Bethesda Softworks ani marką Fallout. Nie używa nazw, kodu, tekstów ani assetów pochodzących z komercyjnych gier. Zasady opisuje [`docs/LEGAL_AND_ASSET_POLICY.md`](docs/LEGAL_AND_ASSET_POLICY.md).

## Aktualny stan

Repozytorium zawiera odtwarzalny fundament techniczny:

- minimalną aplikację obsługującą oba ekrany Nintendo 3DS;
- build `.3dsx` i `.cia`;
- deterministycznie generowaną, oryginalną ikonę, banner i dźwięk bannera;
- RomFS;
- sumy SHA-256 artefaktów;
- GitHub Actions w przypiętym obrazie devkitPro;
- automatyczną kontrolę pochodzenia assetów.

## Budowanie

Wymagane są devkitPro/devkitARM, libctru, Citro3D, Citro2D, Git, hostowe GCC/G++ oraz Python 3.

```sh
make test
make 3dsx
make cia
make release
```

`make release` tworzy:

- `dist/DeepShelter3D.3dsx`;
- `dist/DeepShelter3D.cia`;
- `dist/SHA256SUMS.txt`.

`.cia` jest docelowym formatem instalowanym na odpowiednio skonfigurowanej konsoli homebrew. `.3dsx` pozostaje formatem deweloperskim i awaryjnym dla Homebrew Launcher.

## Sterowanie w wersji startowej

- `A` przełącza diagnostyczne tło dolnego ekranu;
- `START` zamyka aplikację.

## Dokumentacja

- [`docs/LEGAL_AND_ASSET_POLICY.md`](docs/LEGAL_AND_ASSET_POLICY.md)
- [`docs/BUILD_AND_RELEASE.md`](docs/BUILD_AND_RELEASE.md)
- plan prac: [GitHub Issues](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues)
