# Deep Shelter 3D

Oryginalna gra zarządzania podziemnym schronem dla Nintendo 3DS, rozwijana jako aplikacja homebrew w C++ z użyciem devkitARM, libctru, Citro3D i Citro2D.

Projekt nie jest powiązany z Bethesda Softworks ani marką Fallout. Nie używa nazw, kodu, tekstów ani assetów pochodzących z komercyjnych gier. Zasady opisuje [`docs/LEGAL_AND_ASSET_POLICY.md`](docs/LEGAL_AND_ASSET_POLICY.md).

## Aktualny stan

Repozytorium zawiera grywalny pionowy wycinek zarządzania schronem:

- sześć oryginalnych typów pomieszczeń w przestrzennym przekroju 2.5D;
- animowanego mieszkańca, którego można przypisać do wybranego pokoju;
- pętlę `przypisz → produkuj → odbierz → rozbuduj`;
- kontekstowy panel dotykowy z zasobami, postępem i następnym krokiem;
- wersjonowany zapis z CRC, atomową podmianą i kopią zapasową;
- build `.3dsx` i `.cia`;
- deterministycznie generowaną, oryginalną ikonę, banner i dźwięk bannera;
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

## Sterowanie

- `L` / `R` zmienia zaznaczony pokój;
- krzyżak zmienia fokus na dolnym ekranie, a `A` aktywuje akcję;
- ekran dotykowy obsługuje przyciski `PRZYPISZ/ODBIERZ`, `BUDUJ` i `ZAPIS`;
- `B` anuluje bieżącą interakcję;
- Circle Pad przesuwa kamerę, a `X` / `Y` zmienia przybliżenie;
- `SELECT` wczytuje zapis;
- `START` zamyka aplikację.

## Dokumentacja

- [`docs/LEGAL_AND_ASSET_POLICY.md`](docs/LEGAL_AND_ASSET_POLICY.md)
- [`docs/BUILD_AND_RELEASE.md`](docs/BUILD_AND_RELEASE.md)
- [`docs/TRUE_25D_RENDERER.md`](docs/TRUE_25D_RENDERER.md)
- [`docs/GENERATED_DWELLER_ATLAS.md`](docs/GENERATED_DWELLER_ATLAS.md)
- [`docs/PLAYABLE_VERTICAL_SLICE.md`](docs/PLAYABLE_VERTICAL_SLICE.md)
- plan prac: [GitHub Issues](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues)
