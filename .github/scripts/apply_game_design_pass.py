from pathlib import Path

renderer = Path("source/Scene3DRenderer.cpp")
text = renderer.read_text()
if "0xFF56C7F0" in text:
    raise SystemExit(0)

replacements = {
    "constexpr u32 highlight = 0xFF4DB7E8;": "constexpr u32 highlight = 0xFF56C7F0;",
    "rgba(154, 112, 43)": "rgba(194, 139, 48)",
    "rgba(67, 128, 76)": "rgba(82, 158, 91)",
    "rgba(58, 119, 148)": "rgba(67, 151, 188)",
    "rgba(139, 83, 48)": "rgba(174, 101, 52)",
    "rgba(125, 103, 57)": "rgba(164, 132, 65)",
    "rgba(111, 79, 69)": "rgba(151, 99, 82)",
    "floor_index == 1 ? warm_light : steel_mid,": "floor_index == 1 ? rgba(117, 79, 35) : steel_mid,",
    "floor_index == 1 ? rgba(236, 184, 82) : rgba(92, 103, 99),": "floor_index == 1 ? rgba(166, 122, 57) : rgba(82, 93, 90),",
    "const float eye_z = 900.0f / zoom;": "const float eye_z = 1120.0f / zoom;",
    "C3D_AngleFromDegrees(20.0f)": "C3D_AngleFromDegrees(22.0f)",
}
for old, new in replacements.items():
    if old not in text:
        raise SystemExit(f"missing renderer token: {old}")
    text = text.replace(old, new)
renderer.write_text(text)

main = Path("source/main.cpp")
text = main.read_text()
anchor = "void draw_resource(GeneratedUiRenderer& atlas,"
helper = '''const char* room_label(int room_index) {
    switch ((room_index % 6 + 6) % 6) {
        case 0: return "ELEKTROWNIA";
        case 1: return "HYDROPONIKA";
        case 2: return "UZDATNIANIE WODY";
        case 3: return "WARSZTAT";
        case 4: return "MAGAZYN";
        default: return "KWATERY";
    }
}

'''
if "const char* room_label(" not in text:
    if anchor not in text:
        raise SystemExit("missing main anchor")
    text = text.replace(anchor, helper + anchor, 1)
block_start = text.index('    draw_text(buffer, "BIEZACE ZADANIE"')
block_end = text.index("    const int focused_id", block_start)
replacement = '''    draw_text(buffer, "ZAZNACZONY POKOJ", 22.0f, 84.0f, 0.27f,
              C2D_Color32(151, 168, 171, 255));
    draw_text(buffer, room_label(state.selected_room), 22.0f, 98.0f, 0.44f,
              C2D_Color32(246, 193, 82, 255));
    draw_text(buffer, state.message, 22.0f, 118.0f, 0.34f,
              C2D_Color32(244, 239, 220, 255));
    char status[128];
    std::snprintf(status, sizeof(status),
                  "POKOJE %d/6  ZALOGA %d  ZAPAS %d/30",
                  state.rooms, state.workers, state.stored);
    draw_text(buffer, status, 22.0f, 140.0f, 0.31f,
              C2D_Color32(113, 196, 151, 255));
    draw_text(buffer, "D-Pad: pokoj  Pad: kamera  L/R: zoom",
              22.0f, 154.0f, 0.29f,
              C2D_Color32(136, 154, 160, 255));
'''
text = text[:block_start] + replacement + text[block_end:]
main.write_text(text)
