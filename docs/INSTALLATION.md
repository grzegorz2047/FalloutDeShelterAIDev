# Instalacja Deep Shelter 3D

## Format CIA

`DeepShelter3D.cia` jest głównym formatem wydania. Wymaga konsoli Nintendo 3DS z legalnie skonfigurowanym środowiskiem homebrew/CFW i narzędziem instalującym pakiety CIA.

1. Pobierz `DeepShelter3D.cia` oraz `SHA256SUMS.txt` z tego samego GitHub Release.
2. Sprawdź sumę SHA-256 na komputerze.
3. Skopiuj CIA na kartę SD.
4. Zainstaluj pakiet narzędziem dostępnym na własnej konsoli.
5. Uruchom `Deep Shelter 3D` z HOME Menu.

Aktualizacje muszą zachowywać Unique ID `0xD533D`, aby konsola traktowała je jako kolejne wersje tej samej aplikacji.

## Format 3DSX

`DeepShelter3D.3dsx` jest formatem deweloperskim i awaryjnym.

1. Skopiuj plik do katalogu aplikacji Homebrew Launcher na karcie SD.
2. Uruchom Homebrew Launcher.
3. Wybierz `Deep Shelter 3D`.

## Weryfikacja sum kontrolnych

Linux/macOS:

```sh
sha256sum -c SHA256SUMS.txt
```

Windows PowerShell:

```powershell
Get-FileHash .\DeepShelter3D.cia -Algorithm SHA256
Get-FileHash .\DeepShelter3D.3dsx -Algorithm SHA256
```

Porównaj wyniki z `SHA256SUMS.txt`.

## Aktualny stan

Wersje przed `1.0.0` są wydaniami rozwojowymi. Przed aktualizacją ważnego zapisu należy przeczytać changelog i informacje o zgodności zapisu. System trwałego zapisu zostanie dodany w osobnej historyjce.

## Odpowiedzialność użytkownika

Projekt nie dostarcza komercyjnego SDK, kluczy, firmware ani narzędzi obchodzących zabezpieczenia. Użytkownik odpowiada za konfigurację własnego urządzenia i zgodność działań z lokalnym prawem.
