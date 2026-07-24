#include <3ds.h>
#include <citro2d.h>

#include <cstdio>

int main() {
    gfxInitDefault();
    consoleInit(GFX_BOTTOM, nullptr);

    if (!C2D_Init(C2D_DEFAULT_MAX_OBJECTS)) {
        std::printf("Deep Shelter 3D\n\nCitro2D initialization failed.\nPress START to exit.\n");
        while (aptMainLoop()) {
            hidScanInput();
            if (hidKeysDown() & KEY_START) {
                break;
            }
            gspWaitForVBlank();
        }
        gfxExit();
        return 1;
    }

    C2D_Prepare();
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    std::printf("Deep Shelter 3D\n");
    std::printf("Original shelter-management homebrew\n\n");
    std::printf("A: toggle diagnostic panel\n");
    std::printf("START: exit\n");

    bool diagnostic = true;
    while (aptMainLoop()) {
        hidScanInput();
        const u32 down = hidKeysDown();
        if (down & KEY_START) {
            break;
        }
        if (down & KEY_A) {
            diagnostic = !diagnostic;
        }

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(top, C2D_Color32(12, 24, 38, 255));
        C2D_SceneBegin(top);
        C2D_DrawRectSolid(36.0f, 54.0f, 0.0f, 328.0f, 132.0f,
                          C2D_Color32(38, 74, 88, 255));
        C2D_DrawRectSolid(54.0f, 74.0f, 0.0f, 292.0f, 92.0f,
                          C2D_Color32(210, 154, 62, 255));

        C2D_TargetClear(bottom, diagnostic ? C2D_Color32(20, 44, 52, 255)
                                           : C2D_Color32(10, 18, 28, 255));
        C2D_SceneBegin(bottom);
        C3D_FrameEnd(0);
    }

    C2D_Fini();
    gfxExit();
    return 0;
}
