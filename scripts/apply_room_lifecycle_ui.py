from pathlib import Path


def replace_once(text: str, old: str, new: str, label: str) -> str:
    if old not in text:
        raise RuntimeError(f"{label} anchor missing")
    return text.replace(old, new, 1)


path = Path("source/main.cpp")
text = path.read_text()
text = replace_once(
    text,
    "using deep_shelter::gameplay::PrimaryAction;\n",
    "using deep_shelter::gameplay::PrimaryAction;\n"
    "using deep_shelter::gameplay::RoomLifecycleResult;\n",
    "lifecycle using",
)
text = replace_once(
    text,
    """const char* build_result_notice(BuildResult result) {
    switch (result) {
        case BuildResult::Built: return \"Zbudowano wybrany modul.\";
        case BuildResult::NotEnoughCredits: return \"Za malo kredytow.\";
        case BuildResult::Full: return \"Brak wolnego miejsca.\";
        case BuildResult::InvalidPlacement:
            return \"Nie mozna budowac w tej komorce.\";
    }
    return \"Budowa nieudana.\";
}
""",
    """const char* build_result_notice(BuildResult result) {
    switch (result) {
        case BuildResult::Built: return \"Zbudowano wybrany modul.\";
        case BuildResult::NotEnoughCredits: return \"Za malo kredytow.\";
        case BuildResult::Full: return \"Brak wolnego miejsca.\";
        case BuildResult::InvalidPlacement:
            return \"Nie mozna budowac w tej komorce.\";
    }
    return \"Budowa nieudana.\";
}

const char* lifecycle_result_notice(RoomLifecycleResult result) {
    switch (result) {
        case RoomLifecycleResult::Applied: return \"Operacja zakonczona.\";
        case RoomLifecycleResult::MissingRoom: return \"Brak wybranego pokoju.\";
        case RoomLifecycleResult::NotEnoughCredits: return \"Za malo kredytow.\";
        case RoomLifecycleResult::MaximumLevel: return \"Pokoj ma maksymalny poziom.\";
        case RoomLifecycleResult::UnsafeResidents:
            return \"Najpierw ewakuuj mieszkancow lub poczekaj na przejscie.\";
        case RoomLifecycleResult::UnsafeStoredResources:
            return \"Najpierw odbierz lub przenies zasoby.\";
        case RoomLifecycleResult::UnsafeProduction:
            return \"Najpierw zakoncz aktywna produkcje.\";
        case RoomLifecycleResult::LastRoom:
            return \"Nie mozna zburzyc ostatniego pokoju.\";
    }
    return \"Operacja niedostepna.\";
}
""",
    "lifecycle notice",
)
text = replace_once(
    text,
    """        draw_text(buffer, \"POKOJ\", 50.0f, 44.0f, 0.36f,
                  C2D_Color32(151, 168, 171, 255));
""",
    """        char room_caption[32];
        std::snprintf(room_caption,
                      sizeof(room_caption),
                      \"POKOJ L%d  GRUPA x%d\",
                      selected.level,
                      session.selected_group_width());
        draw_text(buffer, room_caption, 50.0f, 44.0f, 0.32f,
                  C2D_Color32(151, 168, 171, 255));
""",
    "room level caption",
)
text = replace_once(
    text,
    """        draw_text(buffer,
                  notice != nullptr ? notice : session.next_step(),
                  68.0f, 126.0f, 0.36f,
                  C2D_Color32(244, 239, 220, 255));
""",
    """        draw_text(buffer,
                  notice != nullptr
                      ? notice
                      : \"L/R POKOJ  X ULEPSZ  Y BURZ\",
                  68.0f, 126.0f, 0.31f,
                  C2D_Color32(244, 239, 220, 255));
""",
    "lifecycle hint",
)
text = replace_once(
    text,
    """        if (held & KEY_X) camera.zoom_by(-0.025f);
        if (held & KEY_Y) camera.zoom_by(0.025f);
""",
    "",
    "release lifecycle keys",
)
text = replace_once(
    text,
    """    bool build_mode = false;

    while (aptMainLoop()) {
""",
    """    bool build_mode = false;
    bool upgrade_confirmation = false;
    bool demolition_confirmation = false;

    while (aptMainLoop()) {
""",
    "confirmation state",
)
text = replace_once(
    text,
    """            if (down & KEY_SELECT) {
""",
    """            bool lifecycle_key_consumed = false;
            if ((down & KEY_B) &&
                (upgrade_confirmation || demolition_confirmation)) {
                upgrade_confirmation = false;
                demolition_confirmation = false;
                lifecycle_key_consumed = true;
                set_notice(notice, sizeof(notice), \"Anulowano operacje pokoju.\");
                notice_until_ms = current_frame_ms + 1400u;
            }
            if (down & KEY_X) {
                lifecycle_key_consumed = true;
                if (upgrade_confirmation) {
                    const RoomLifecycleResult result =
                        session.confirm_upgrade_selected();
                    upgrade_confirmation = false;
                    set_notice(notice,
                               sizeof(notice),
                               lifecycle_result_notice(result));
                    notice_until_ms = current_frame_ms + 1800u;
                } else {
                    const auto preview = session.preview_upgrade_selected();
                    if (preview.allowed()) {
                        demolition_confirmation = false;
                        upgrade_confirmation = true;
                        std::snprintf(notice,
                                      sizeof(notice),
                                      \"ULEPSZ x%d: koszt %d KR. X TAK, B NIE.\",
                                      preview.group_width,
                                      -preview.credit_delta);
                        notice_until_ms = current_frame_ms + 60000u;
                    } else {
                        set_notice(notice,
                                   sizeof(notice),
                                   lifecycle_result_notice(preview.result));
                        notice_until_ms = current_frame_ms + 1800u;
                    }
                }
            }
            if (down & KEY_Y) {
                lifecycle_key_consumed = true;
                if (demolition_confirmation) {
                    const RoomLifecycleResult result =
                        session.confirm_demolish_selected();
                    demolition_confirmation = false;
                    set_notice(notice,
                               sizeof(notice),
                               lifecycle_result_notice(result));
                    notice_until_ms = current_frame_ms + 1800u;
                    center_on_selected(camera, session);
                    ui_availability = UiAvailability{};
                } else {
                    const auto preview = session.preview_demolish_selected();
                    if (preview.allowed()) {
                        upgrade_confirmation = false;
                        demolition_confirmation = true;
                        std::snprintf(notice,
                                      sizeof(notice),
                                      \"BURZ: zwrot %d KR. Y TAK, B NIE.\",
                                      preview.credit_delta);
                        notice_until_ms = current_frame_ms + 60000u;
                    } else {
                        set_notice(notice,
                                   sizeof(notice),
                                   lifecycle_result_notice(preview.result));
                        notice_until_ms = current_frame_ms + 1800u;
                    }
                }
            }

            if (down & KEY_SELECT) {
""",
    "lifecycle keyboard handling",
)
text = replace_once(
    text,
    """            sync_ui_availability(ui, session, ui_availability);
            const auto action = ui.route(read_ui_input(down, held, up));
            if (action && action->type == UiActionType::Activate) {
""",
    """            sync_ui_availability(ui, session, ui_availability);
            const auto action =
                lifecycle_key_consumed || upgrade_confirmation ||
                        demolition_confirmation
                    ? std::optional<deep_shelter::ui::UiAction>{}
                    : ui.route(read_ui_input(down, held, up));
            if (action && action->type == UiActionType::Activate) {
""",
    "modal blocks standard UI",
)
path.write_text(text)
