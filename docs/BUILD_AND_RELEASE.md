# Budowanie i wydawanie Deep Shelter 3D

## Artefakty

Projekt tworzy dwa formaty:

1. `DeepShelter3D.cia` — główny artefakt instalowany na odpowiednio skonfigurowanej konsoli Nintendo 3DS z CFW/homebrew.
2. `DeepShelter3D.3dsx` — format deweloperski i awaryjny uruchamiany przez Homebrew Launcher.

Oba formaty używają tego samego kodu i RomFS.

## Łańcuch budowania

1. walidacja polityki assetów;
2. generowanie oryginalnej ikony, bannera i WAV;
3. pobranie przypiętych wersji `makerom` i `bannertool`;
4. kompilacja C++ przez devkitARM;
5. linkowanie ELF;
6. utworzenie SMDH i `.3dsx` z RomFS;
7. przygotowanie kompatybilnego RSF;
8. utworzenie `.cia` z ExeFS i RomFS;
9. obliczenie i sprawdzenie SHA-256.

## Polecenia

```sh
make test
make 3dsx
make cia
make release
make clean
```

`make release` zapisuje gotowe pliki w `dist/`.

## Przypięte zależności

- obraz CI: `devkitpro/devkitarm:20260610`;
- makerom: tag `makerom-v0.18.4`;
- bannertool: commit `16d8c5a0ce02a5e06e64ab42275132fca57c04a2`.

Nie wolno zastępować ich nieprzypiętą wersją bez osobnego PR, uzasadnienia i pełnego testu obu formatów.

## Metadane

- nazwa: `Deep Shelter 3D`;
- autor: `grzegorz2047`;
- ProductCode: `CTR-H-DS3D`;
- stabilny Unique ID: `0xD533D`.

Unique ID nie może zmieniać się pomiędzy zwykłymi aktualizacjami, ponieważ konsola potraktuje paczkę jako osobną aplikację.

## Zasady bezpieczeństwa

- repozytorium nie zawiera prywatnych kluczy ani materiałów z komercyjnego SDK;
- CIA jest paczką homebrew bez szyfrowania i komercyjnego podpisu;
- wygenerowane assety wydania są oryginalne i deterministyczne;
- `.tools/`, artefakty i pliki pośrednie nie są commitowane;
- release musi zawierać oba formaty, instrukcję i sumy kontrolne.

## Test sprzętowy

Automatyczny build nie zastępuje testu fizycznego. Przed stabilnym wydaniem trzeba potwierdzić:

- instalację CIA na prawdziwym 3DS;
- start i zamknięcie aplikacji;
- uruchomienie 3DSX przez Homebrew Launcher;
- aktualizację CIA z zachowaniem tego samego Unique ID;
- zachowanie zapisu po aktualizacji, gdy system zapisu zostanie dodany.
