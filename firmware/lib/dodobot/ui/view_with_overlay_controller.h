#include "ui/view_controller.h"

class ViewWithOverlayController : public ViewController
{
public:
    ViewWithOverlayController(ViewController* overlay) : ViewController() {
        this.overlay = overlay;
    }
    void draw() {
        overlay->draw();
        draw_with_overlay();
    }
    void on_load() {
        overlay->on_load();
        on_load_with_overlay();
    }
    void on_unload() {
        overlay->on_unload();
        on_unload_with_overlay();
    }

    void draw_with_overlays()  {}
    void on_load_with_overlays()  {}
    void on_unload_with_overlays()  {}
private:
    ViewController overlay;
};
