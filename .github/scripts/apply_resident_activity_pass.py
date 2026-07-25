from pathlib import Path

path = Path("source/Scene3DRenderer.cpp")
text = path.read_text()
replacements = [
    ("const u32 frame = selected ? rgba(162, 113, 38) : rgba(55, 65, 66);",
     "const u32 frame = selected ? rgba(126, 92, 40) : rgba(55, 65, 66);"),
    ("selected ? rgba(229, 180, 76) : accent,",
     "selected ? rgba(198, 151, 62) : accent,"),
    ("const u32 rock = rgba(42, 38, 36);\n    const u32 cut_rock = rgba(57, 51, 47);\n    const u32 brace = rgba(66, 74, 73);",
     "const u32 rock = rgba(34, 31, 30);\n    const u32 cut_rock = rgba(45, 41, 39);\n    const u32 brace = rgba(54, 61, 61);"),
    ("case 0: return rgba(194, 139, 48);", "case 0: return rgba(126, 91, 34);"),
    ("case 1: return rgba(82, 158, 91);", "case 1: return rgba(58, 105, 65);"),
    ("case 2: return rgba(67, 151, 188);", "case 2: return rgba(49, 101, 124);"),
    ("case 3: return rgba(174, 101, 52);", "case 3: return rgba(113, 70, 42);"),
    ("case 4: return rgba(164, 132, 65);", "case 4: return rgba(108, 88, 48);"),
    ("default: return rgba(151, 99, 82);", "default: return rgba(99, 68, 59);"),
    ("""    if (resident) {
        mesh_.append_box({x + 88.0f, y + 26.0f, -2.0f, 11.0f, 13.0f, 5.0f,
                          rgba(255, 225, 191), assets::GeneratedMaterial::Steel});
        mesh_.append_box({x + 84.0f, y + 39.0f, -2.0f, 19.0f, 17.0f, 5.0f,
                          rgba(102, 204, 218), assets::GeneratedMaterial::Steel});
    }
""",
     """    if (resident) {
        const float resident_x = profile == 0 ? 105.0f :
                                 (profile == 1 ? 88.0f :
                                  (profile == 2 ? 18.0f :
                                   (profile == 3 ? 64.0f :
                                    (profile == 4 ? 12.0f : 92.0f))));
        mesh_.append_box({x + resident_x + 4.0f, y + 17.0f, -1.0f, 10.0f, 10.0f, 6.0f,
                          rgba(245, 210, 174), assets::GeneratedMaterial::Steel});
        mesh_.append_box({x + resident_x, y + 27.0f, -1.0f, 18.0f, 27.0f, 6.0f,
                          rgba(48, 119, 164), assets::GeneratedMaterial::ControlPanel});
    }
"""),
    ("mesh_.append_box({371.0f, 5.0f, -27.0f, 29.0f, 67.0f, 16.0f,",
     "mesh_.append_box({368.0f, 5.0f, -27.0f, 32.0f, 62.0f, 16.0f,"),
    ("mesh_.append_box({365.0f, 66.0f, -31.0f, 35.0f, 92.0f, 20.0f,",
     "mesh_.append_box({360.0f, 61.0f, -31.0f, 40.0f, 98.0f, 20.0f,"),
    ("mesh_.append_box({374.0f, 151.0f, -26.0f, 26.0f, 81.0f, 15.0f,",
     "mesh_.append_box({372.0f, 154.0f, -26.0f, 28.0f, 78.0f, 15.0f,"),
]
for old, new in replacements:
    if old not in text:
        raise SystemExit(f"missing expected source fragment: {old[:80]}")
    text = text.replace(old, new, 1)
path.write_text(text)
