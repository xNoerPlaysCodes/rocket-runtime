#include "rocket/asset.hpp"
#include "rocket/renderer.hpp"
#include "rocket/types.hpp"
#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QFont>
#include <QtOpenGLWidgets/qopenglwidget.h>
#include <algorithm>
#include <iostream>
#include <map>
#include <mutex>
#include <qapplication.h>
#include <qcolordialog.h>
#include <qfiledialog.h>
#include <qformlayout.h>
#include <qmessagebox.h>
#include <qopenglcontext_platform.h>
#include <qpainter.h>
#include <qplaintextedit.h>
#include <qscrollarea.h>
#include <qshortcut.h>
#include <qsplitter.h>
#include <qtabwidget.h>
#include <qtoolbutton.h>
#include <rocket/modularity/window_backend.hpp>
#include <rocket/window.hpp>
#include <rocket/runtime.hpp>
#include <QOpenGLContext>
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <string>
#include <QStyle>
#include <QLineEdit>
#include <QFrame>
#include <QDir>
#include <QFile>
#include <nlohmann/json.hpp>
#include <fstream>
#include <optional>
#include <QSvgRenderer>
#include <QClipboard>

#include "code_generator.hpp"

struct drawcall_t {
    int order;
    std::function<void(rocket::renderer_2d *ren, rocket::window_backend_i *win)> cb;
};

namespace rocket {

class qt_widget_t : public QOpenGLWidget, public window_backend_i {
    Q_OBJECT
private:
    rocket::renderer_2d *ren;
    std::map<std::string, drawcall_t> drawcalls;
public:
    ~qt_widget_t() override { destructor_called = true; }

    void set_size(const rocket::vec2i_t &s) override { window_backend_i::size = s; }
    void set_title(const std::string &t) override { title = t; }
    std::string get_title() const override { return title; }
    rocket::vec2i_t get_size() const override { return window_backend_i::size; }
    void set_cursor_mode(cursor_mode_t m) override { mode = m; }
    cursor_mode_t get_cursor_mode() const override { return mode; }
    void set_cursor_position(const rocket::vec2d_t &pos) const override {}
    rocket::vec2d_t get_cursor_position() const override { return {0.0, 0.0}; }
    bool is_running() const override { return !destructor_called; }
    double get_time() const override { return 0.0; }
    window_state_t get_window_state() const override { return {}; }
    void set_window_state(window_state_t state) const override {}

    platform_t get_platform() const override {
        platform_t platform;
        QByteArray arr = qgetenv("QT_QPA_PLATFORM");
        platform.name = "Qt + " + (arr.isNull() ? "Unknown" : arr.toStdString());
        platform.rge_name = "Qt::Desktop";
#ifdef ROCKETGE__Platform_Linux
        platform.os_name = "Linux";
#elifdef ROCKETGE__Platform_macOS
        platform.os_name = "macOS";
#elifdef ROCKETGE__Platform_Windows
        platform.os_name = "Windows";
#else
        platform.os_name = "Unknown";
#endif
        platform.type = platform_type_t::unknown;
        return platform;
    }

protected:
    void swap_buffers() const override {}

public:
    void register_on_close(std::function<void()> fn) override {}
    void poll_events() override {}
    void set_vsync(bool) override {}
    void close() override {}
    std::shared_ptr<rocket::texture_t> get_icon() const override { return icon; }
    void set_icon(std::shared_ptr<rocket::texture_t> icn) override { this->icon = icn; }

    qt_widget_t(QWidget *parent = nullptr) : QOpenGLWidget(parent) {}

    void register_drawcall(const std::string& name, int order, std::function<void(rocket::renderer_2d*, rocket::window_backend_i*)> cb) {
        drawcalls[name] = drawcall_t{ order, cb };
    }
    void replace_drawcall(const std::string& name, int order, std::function<void(rocket::renderer_2d*, rocket::window_backend_i*)> cb) {
        drawcalls[name] = drawcall_t{ order, cb };
    }
    drawcall_t get_drawcall(const std::string& name) {
        auto it = drawcalls.find(name);
        if (it == drawcalls.end()) return {0, nullptr};
        return it->second;
    }
    void remove_drawcall(std::string name) {
        auto it = drawcalls.find(name);
        if (it != drawcalls.end()) drawcalls.erase(it);
    }

protected:
    void initializeGL() override {
        this->set_title("RocketGE - Editor");
        this->makeCurrent();
        rocket::log("Window created as [" + std::to_string(window_backend_i::size.x) + "x" + std::to_string(window_backend_i::size.y) + "]: " + title,
            "qt_widget_t", "constructor", "info");
        auto platform = get_platform();
        for (auto &l : std::vector<std::string>{
            "RocketGE v" ROCKETGE__VERSION,
            "Windowing: " + platform.name,
            "Platform: "  + platform.rge_name,
        }) rocket::log(l, "qt_widget_t", "constructor", "info");
        ren = new renderer_2d(this, 60, { .show_splash = false });
    }

    void paintGL_paused() {
        ren->begin_frame();
        ren->clear(rocket::rgba_color::black());
        {
            rocket::text_t text = { "Paused", 32, rocket::rgba_color::white() };
            auto vp_size  = ren->get_viewport_size();
            auto txt_size = text.measure();
            ren->draw_text(text, vp_size / 2 - txt_size / 2);
        }
        ren->end_frame();
        this->update();
    }

    void paintGL() override {
        if (paused) { paintGL_paused(); return; }

        ren->begin_frame();
        ren->clear();

        std::vector<drawcall_t*> ordered;
        for (auto &[name, entry] : drawcalls)
            ordered.push_back(&entry);

        std::sort(ordered.begin(), ordered.end(), [](auto *a, auto *b) {
            if (a->order != b->order) return a->order < b->order;
            return a < b;
        });

        for (auto *entry : ordered)
            if (entry->cb) entry->cb(ren, this);

        ren->draw_fps();
        ren->end_frame();
        this->update();
    }

    void resizeGL(int w, int h) override {
        ren->set_viewport_size({ w * 1.f, h * 1.f });
        this->window_backend_i::size = { w, h };
    }

public:
    bool paused = false;
};

} // namespace rocket

int global_id = 0;

struct command_t {
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual ~command_t() = default;
};

struct command_history_t {
    std::vector<std::unique_ptr<command_t>> undo_stack;
    std::vector<std::unique_ptr<command_t>> redo_stack;

    void push(std::unique_ptr<command_t> cmd) {
        cmd->execute();
        undo_stack.push_back(std::move(cmd));
        redo_stack.clear();
    }
    void undo() {
        if (undo_stack.empty()) return;
        auto cmd = std::move(undo_stack.back());
        undo_stack.pop_back();
        cmd->undo();
        redo_stack.push_back(std::move(cmd));
    }
    void redo() {
        if (redo_stack.empty()) return;
        auto cmd = std::move(redo_stack.back());
        redo_stack.pop_back();
        cmd->execute();
        undo_stack.push_back(std::move(cmd));
    }
    bool can_undo() const { return !undo_stack.empty(); }
    bool can_redo() const { return !redo_stack.empty(); }
};

command_history_t history;

struct data_1_t {};
struct data_2_t {};

struct game_object_t {
    int id = -1;
    std::string name = "Newly Created Game Object";
    rocket::vec2f_t position;
    rocket::vec2f_t size;
    rocket::rgba_color color;
    float rotation   = 0;
    int draw_order   = 0;
    std::shared_ptr<rocket::texture_t> texture = nullptr;
    data_1_t *data_1 = nullptr;
    data_2_t *data_2 = nullptr;
};

struct object_snapshot_cmd_t : command_t {
    std::vector<game_object_t> *game_objects;
    game_object_t before, after;
    void apply(const game_object_t &s) {
        for (auto &o : *game_objects)
            if (o.id == s.id) { o = s; return; }
    }
    void execute() override { apply(after);  }
    void undo()    override { apply(before); }
};

struct delete_object_cmd_t : command_t {
    std::vector<game_object_t> *game_objects;
    game_object_t saved;
    std::function<void(const game_object_t&)> on_restore;
    std::function<void(int)> on_remove;
    void execute() override { on_remove(saved.id); }
    void undo()    override { on_restore(saved);   }
};

struct create_object_cmd_t : command_t {
    std::vector<game_object_t> *game_objects;
    std::function<void()> on_create;
    std::function<void(int)> on_remove;
    int created_id = -1;
    void execute() override { on_create(); created_id = game_objects->back().id; }
    void undo()    override { on_remove(created_id); }
};

QIcon make_code_icon() {
    const char *svg = R"(
        <svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
            <polyline points="8 6 2 12 8 18" fill="none" stroke="white" stroke-width="2"/>
            <polyline points="16 6 22 12 16 18" fill="none" stroke="white" stroke-width="2"/>
            <line x1="14" y1="4" x2="10" y2="20" stroke="white" stroke-width="2"/>
        </svg>
    )";
    QSvgRenderer renderer = {QByteArray(svg)};
    QPixmap pm(18, 18);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    renderer.render(&p);
    return QIcon(pm);
}

int main(int argc, char *argv[]) {
#ifdef ROCKETGE__Platform_Linux
    qputenv("QT_QPA_PLATFORM", "xcb");
    qputenv("QT_XCB_GL_INTEGRATION", "xcb_glx");
#endif

    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);

    QSurfaceFormat fmt;
    fmt.setVersion(4, 1);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("RocketGE - Editor");

    QStackedWidget *stack = new QStackedWidget;
    QVBoxLayout *root_layout = new QVBoxLayout(&window);
    root_layout->setContentsMargins(0, 0, 0, 0);
    root_layout->addWidget(stack);

    // -------------------------
    // Menu page
    // -------------------------
    QWidget *menu_page = new QWidget;
    QHBoxLayout *main_layout = new QHBoxLayout(menu_page);

    QPushButton *new_button  = new QPushButton("New");
    QPushButton *open_button = new QPushButton("Open");
    QPushButton *_Debug_new = new QPushButton("Debug New");
    QLabel *label = new QLabel("RocketGE - Editor");
    QFont font; font.setPointSize(20); label->setFont(font);

    new_button->setStyleSheet("min-height: 45px;");
    open_button->setStyleSheet("min-height: 45px;");

    QWidget *sidebar = new QWidget;
    sidebar->setFixedWidth(300);
    QVBoxLayout *sidebar_layout = new QVBoxLayout(sidebar);
    sidebar_layout->addWidget(label);
    sidebar_layout->addWidget(new_button);
    sidebar_layout->addWidget(open_button);
    sidebar_layout->addWidget(_Debug_new);
    sidebar_layout->addStretch();

    QWidget *main_content = new QWidget;
    main_content->setStyleSheet("background-color: rgb(14,14,14); border-radius: 14px;");
    main_layout->addWidget(sidebar);
    main_layout->addWidget(main_content, 1);

    // -------------------------
    // Editor page
    // -------------------------
    QWidget *editor_page = new QWidget;
    QVBoxLayout *editor_layout = new QVBoxLayout(editor_page);
    editor_layout->setContentsMargins(0, 0, 0, 0);
    editor_layout->setSpacing(0);

    // --- Toolbar ---
    auto make_toolbar_btn = [](const QString &tooltip, std::optional<QStyle::StandardPixmap> icon = std::nullopt) -> QToolButton* {
        QToolButton *btn = new QToolButton;
        if (icon != std::nullopt)
            btn->setIcon(QApplication::style()->standardIcon(*icon));
        btn->setToolTip(tooltip);
        btn->setFixedSize(28, 28);
        btn->setIconSize(QSize(18, 18));
        btn->setStyleSheet(R"(
            QToolButton {
                background-color: transparent;
                border: 1px solid transparent;
                border-radius: 3px;
            }
            QToolButton:hover {
                background-color: rgb(70,70,70);
                border-color: #666;
            }
            QToolButton:pressed { background-color: rgb(50,50,50); }
            QToolButton:checked {
                background-color: rgb(50,100,200);
                border-color: #88aaff;
            }
        )");
        return btn;
    };

    auto make_separator = []() -> QFrame* {
        QFrame *sep = new QFrame;
        sep->setFrameShape(QFrame::VLine);
        sep->setFixedWidth(1);
        sep->setStyleSheet("background-color: #555;");
        return sep;
    };

    QWidget *toolbar = new QWidget;
    toolbar->setFixedHeight(36);
    toolbar->setStyleSheet("background-color: rgb(45,45,45); border-bottom: 1px solid #333;");
    QHBoxLayout *toolbar_layout = new QHBoxLayout(toolbar);
    toolbar_layout->setContentsMargins(6, 2, 6, 2);
    toolbar_layout->setSpacing(4);

    QToolButton *btn_new      = make_toolbar_btn("New Scene",     QStyle::SP_FileIcon);
    QToolButton *btn_open     = make_toolbar_btn("Open Scene",    QStyle::SP_DirOpenIcon);
    QToolButton *btn_save     = make_toolbar_btn("Save Scene",    QStyle::SP_DialogSaveButton);
    QToolButton *btn_undo     = make_toolbar_btn("Undo",          QStyle::SP_ArrowBack);
    QToolButton *btn_redo     = make_toolbar_btn("Redo",          QStyle::SP_ArrowForward);
    QToolButton *btn_pause    = make_toolbar_btn("Pause",         QStyle::SP_MediaPause);
    QToolButton *btn_codegen  = make_toolbar_btn("Generate Code");
    btn_codegen->setIcon(make_code_icon());
    btn_pause->setCheckable(true);

    toolbar_layout->addWidget(btn_new);
    toolbar_layout->addWidget(btn_open);
    toolbar_layout->addWidget(btn_save);
    toolbar_layout->addWidget(make_separator());
    toolbar_layout->addWidget(btn_undo);
    toolbar_layout->addWidget(btn_redo);
    toolbar_layout->addWidget(make_separator());
    toolbar_layout->addWidget(btn_pause);
    toolbar_layout->addWidget(make_separator());
    toolbar_layout->addWidget(btn_codegen);
    toolbar_layout->addStretch();

    editor_layout->addWidget(toolbar);

    // --- Splitter ---
    QSplitter *splitter = new QSplitter(Qt::Horizontal);

    rocket::init(argc, argv);
    rocket::qt_widget_t *viewport = new rocket::qt_widget_t(splitter);
    viewport->resize({ 1, 1 });
    viewport->show();

    QWidget *inspector = new QWidget;
    QVBoxLayout *inspector_layout = new QVBoxLayout(inspector);
    inspector->setStyleSheet("background-color: rgb(40,40,40);");

    splitter->addWidget(inspector);
    splitter->addWidget(viewport);
    splitter->setHandleWidth(6);
    splitter->setSizes({350, 1200});

    editor_layout->addWidget(splitter, 1);

    stack->addWidget(menu_page);
    stack->addWidget(editor_page);

    // -------------------------
    // Object list
    // -------------------------
    QWidget *objects_content = new QWidget;
    QVBoxLayout *objects_layout = new QVBoxLayout(objects_content);
    objects_layout->setContentsMargins(3, 3, 3, 3);
    objects_layout->setSpacing(5);

    QScrollArea *objects_scroll = new QScrollArea;
    objects_scroll->setWidget(objects_content);
    objects_scroll->setWidgetResizable(true);
    objects_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    objects_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    std::vector<game_object_t> game_objects;
    rocket::asset_manager_t *am = new rocket::asset_manager_t;

    QString current_project_dir = "";
    QString current_scene_path  = "";

    // -------------------------
    // register_obj
    // -------------------------
    auto register_obj = [&game_objects, &objects_layout, &viewport, am, btn_undo, btn_redo](std::optional<game_object_t> preset = std::nullopt) -> size_t {
        game_objects.emplace_back();
        game_object_t &obj = game_objects.back();
        if (preset.has_value()) {
            obj = preset.value();
        } else {
            obj.id         = global_id++;
            obj.draw_order = obj.id;
        }
        size_t obj_idx = game_objects.size() - 1;

        QWidget *header_row = new QWidget;
        QHBoxLayout *header_layout = new QHBoxLayout(header_row);
        header_layout->setContentsMargins(0, 0, 0, 0);

        QLabel *name_label = new QLabel(QString::fromStdString(obj.name));

        auto make_btn = [](QStyle::StandardPixmap icon, const QString &tip, bool checkable) -> QToolButton* {
            QToolButton *b = new QToolButton;
            b->setIcon(QApplication::style()->standardIcon(icon));
            b->setToolTip(tip);
            b->setFixedSize(24, 24);
            b->setIconSize(QSize(16, 16));
            b->setCheckable(checkable);
            b->setStyleSheet(R"(
                QToolButton { background-color: rgb(70,70,70); border: 1px solid #555; border-radius: 3px; }
                QToolButton:hover { background-color: rgb(90,90,110); border-color: #888; }
                QToolButton:checked { background-color: rgb(50,100,200); border-color: #88aaff; }
            )");
            return b;
        };

        QToolButton *edit_btn = make_btn(QStyle::SP_FileDialogDetailedView, "Edit",   true);
        QToolButton *del_btn  = make_btn(QStyle::SP_TrashIcon,              "Delete", false);

        header_layout->addWidget(name_label, 1);
        header_layout->addWidget(edit_btn);
        header_layout->addWidget(del_btn);

        QWidget *obj_content = new QWidget;
        QFormLayout *form = new QFormLayout(obj_content);
        form->setContentsMargins(4, 4, 4, 4);

        QLineEdit *id_field = new QLineEdit(QString::number(obj.id));
        id_field->setReadOnly(true); id_field->setDisabled(true);
        form->addRow("ID:", id_field);

        QLineEdit *name_field = new QLineEdit(QString::fromStdString(obj.name));
        name_field->setReadOnly(true);
        form->addRow("Name:", name_field);

        auto make_hpair = [](QLineEdit *&a, QLineEdit *&b, const QString &la, const QString &lb, float va, float vb) -> QWidget* {
            a = new QLineEdit(QString::number(va));
            b = new QLineEdit(QString::number(vb));
            a->setReadOnly(true); b->setReadOnly(true);
            QWidget *row = new QWidget;
            QHBoxLayout *l = new QHBoxLayout(row);
            l->setContentsMargins(0,0,0,0);
            l->addWidget(new QLabel(la)); l->addWidget(a);
            l->addWidget(new QLabel(lb)); l->addWidget(b);
            return row;
        };

        QLineEdit *pos_x, *pos_y, *size_w, *size_h;
        form->addRow("Position:", make_hpair(pos_x, pos_y, "X:", "Y:", obj.position.x, obj.position.y));
        form->addRow("Size:",     make_hpair(size_w, size_h, "W:", "H:", obj.size.x, obj.size.y));

        QLineEdit *draw_order_field = new QLineEdit(QString::number(obj.draw_order));
        draw_order_field->setReadOnly(true);
        form->addRow("Draw Order:", draw_order_field);

        QLineEdit *rot_field = new QLineEdit(QString::number(obj.rotation));
        rot_field->setReadOnly(true);
        form->addRow("Rotation:", rot_field);

        QLineEdit *texture_field = new QLineEdit(obj.texture ? QString::fromStdString(obj.texture->path) : "");
        texture_field->setReadOnly(true);
        texture_field->setPlaceholderText("No texture");
        QPushButton *texture_browse_btn = new QPushButton("...");
        texture_browse_btn->setFixedWidth(28);
        texture_browse_btn->setEnabled(false);
        QWidget *tex_row = new QWidget;
        QHBoxLayout *tex_layout = new QHBoxLayout(tex_row);
        tex_layout->setContentsMargins(0,0,0,0);
        tex_layout->addWidget(texture_field, 1);
        tex_layout->addWidget(texture_browse_btn);
        form->addRow("Texture:", tex_row);

        auto make_color_style = [](const rocket::rgba_color &c) -> QString {
            return QString("background-color: rgba(%1,%2,%3,%4); border: 1px solid #888;")
                .arg((int)c.x).arg((int)c.y).arg((int)c.z).arg((int)c.w);
        };
        QPushButton *color_btn = new QPushButton;
        color_btn->setStyleSheet(make_color_style(obj.color));
        color_btn->setFixedHeight(24);
        color_btn->setEnabled(false);
        form->addRow("Color:", color_btn);

        QObject::connect(texture_browse_btn, &QPushButton::clicked, [&game_objects, obj_idx, texture_field, &viewport, am]() {
            QString path = QFileDialog::getOpenFileName(nullptr, "Select Texture", "", "Images (*.png *.jpg *.jpeg *.bmp *.tga)");
            if (path.isEmpty()) return;
            game_object_t &o = game_objects[obj_idx];
            o.texture = am->get_texture(am->load_texture(path.toStdString()));
            texture_field->setText(path);
            viewport->replace_drawcall("draw_obj_with_id=" + std::to_string(o.id), o.draw_order,
                [&game_objects, obj_idx](rocket::renderer_2d *ren, rocket::window_backend_i *win) {
                    const game_object_t &obj = game_objects[obj_idx];
                    if (obj.texture) ren->draw_texture(obj.texture, {obj.position, obj.size}, obj.rotation);
                    else             ren->draw_rectangle(obj.position, obj.size, obj.color, obj.rotation);
                });
        });

        QObject::connect(color_btn, &QPushButton::clicked, [&game_objects, obj_idx, color_btn, make_color_style, &viewport]() {
            game_object_t &o = game_objects[obj_idx];
            QColor picked = QColorDialog::getColor(
                QColor((int)o.color.x, (int)o.color.y, (int)o.color.z, (int)o.color.w),
                nullptr, "Pick Color", QColorDialog::ShowAlphaChannel);
            if (!picked.isValid()) return;
            o.color = { (uint8_t)picked.red(), (uint8_t)picked.green(), (uint8_t)picked.blue(), (uint8_t)picked.alpha() };
            color_btn->setStyleSheet(make_color_style(o.color));
            viewport->replace_drawcall("draw_obj_with_id=" + std::to_string(o.id), o.draw_order,
                [&game_objects, obj_idx](rocket::renderer_2d *ren, rocket::window_backend_i *win) {
                    const game_object_t &obj = game_objects[obj_idx];
                    if (!obj.texture) ren->draw_rectangle(obj.position, obj.size, obj.color, obj.rotation);
                });
        });

        auto set_editing = [=](bool editing) {
            auto set_field = [=](QLineEdit *f, bool editable) {
                f->setReadOnly(!editable);
                f->setFocusPolicy(editable ? Qt::StrongFocus : Qt::NoFocus);
                f->setStyleSheet(editable
                    ? "background-color: rgb(60,60,80); color: white;"
                    : "background-color: transparent; color: #aaa; border: none;");
            };
            set_field(name_field,       editing);
            set_field(pos_x,            editing);
            set_field(pos_y,            editing);
            set_field(size_w,           editing);
            set_field(size_h,           editing);
            set_field(rot_field,        editing);
            set_field(draw_order_field, editing);
            set_field(texture_field,    editing);
            texture_browse_btn->setEnabled(editing);
            color_btn->setEnabled(editing);
        };

        set_editing(false);

        QObject::connect(edit_btn, &QToolButton::toggled, [=, &game_objects](bool editing) {
            set_editing(editing);
            if (!editing) {
                game_object_t before = game_objects[obj_idx];
                game_object_t after  = before;
                after.name       = name_field->text().toStdString();
                after.position   = { pos_x->text().toFloat(),       pos_y->text().toFloat() };
                after.size       = { size_w->text().toFloat(),       size_h->text().toFloat() };
                after.rotation   = rot_field->text().toFloat();
                after.draw_order = draw_order_field->text().toInt();

                auto cmd = std::make_unique<object_snapshot_cmd_t>();
                cmd->game_objects = &game_objects;
                cmd->before = before;
                cmd->after  = after;
                history.push(std::move(cmd));

                const game_object_t &o = game_objects[obj_idx];
                name_label->setText(QString::fromStdString(o.name));
                btn_undo->setEnabled(history.can_undo());
                btn_redo->setEnabled(history.can_redo());

                viewport->replace_drawcall("draw_obj_with_id=" + std::to_string(o.id), o.draw_order,
                    [&game_objects, obj_idx](rocket::renderer_2d *ren, rocket::window_backend_i *win) {
                        const game_object_t &obj = game_objects[obj_idx];
                        if (obj.texture) ren->draw_texture(obj.texture, {obj.position, obj.size}, obj.rotation);
                        else             ren->draw_rectangle(obj.position, obj.size, obj.color, obj.rotation);
                    });
            }
        });

        QWidget *section = new QWidget;
        QVBoxLayout *section_layout = new QVBoxLayout(section);
        section_layout->setContentsMargins(0, 0, 0, 0);
        section_layout->setSpacing(0);

        QToolButton *collapse_btn = new QToolButton;
        collapse_btn->setCheckable(true);
        collapse_btn->setChecked(true);
        collapse_btn->setArrowType(Qt::DownArrow);
        collapse_btn->setToolButtonStyle(Qt::ToolButtonIconOnly);

        QWidget *title_bar = new QWidget;
        title_bar->setStyleSheet("background-color: rgb(55,55,55);");
        QHBoxLayout *title_bar_layout = new QHBoxLayout(title_bar);
        title_bar_layout->setContentsMargins(4, 2, 4, 2);
        title_bar_layout->addWidget(collapse_btn);
        title_bar_layout->addWidget(header_row, 1);

        section_layout->addWidget(title_bar);
        section_layout->addWidget(obj_content);

        QObject::connect(collapse_btn, &QToolButton::toggled, [=](bool open) {
            obj_content->setVisible(open);
            collapse_btn->setArrowType(open ? Qt::DownArrow : Qt::RightArrow);
        });

        QObject::connect(del_btn, &QToolButton::clicked, [=, &game_objects]() {
            game_object_t saved = game_objects[obj_idx];
            auto cmd = std::make_unique<delete_object_cmd_t>();
            cmd->game_objects = &game_objects;
            cmd->saved = saved;

            cmd->on_remove = [=, &game_objects](int id) {
                viewport->remove_drawcall("draw_obj_with_id=" + std::to_string(id));
                auto it = std::find_if(game_objects.begin(), game_objects.end(),
                    [id](const game_object_t &o) { return o.id == id; });
                if (it != game_objects.end()) game_objects.erase(it);
                section->hide();
            };

            cmd->on_restore = [=, &game_objects](const game_object_t &obj) {
                game_objects.push_back(obj);
                viewport->register_drawcall("draw_obj_with_id=" + std::to_string(obj.id), obj.draw_order,
                    [=, &game_objects](rocket::renderer_2d *ren, rocket::window_backend_i *win) {
                        auto it = std::find_if(game_objects.begin(), game_objects.end(),
                            [&obj](const game_object_t &o) { return o.id == obj.id; });
                        if (it == game_objects.end()) return;
                        if (it->texture) ren->draw_texture(it->texture, {it->position, it->size}, it->rotation);
                        else             ren->draw_rectangle(it->position, it->size, it->color, it->rotation);
                    });
                section->show();
            };

            history.push(std::move(cmd));
            btn_undo->setEnabled(history.can_undo());
            btn_redo->setEnabled(history.can_redo());
        });

        objects_layout->insertWidget(0, section);

        viewport->register_drawcall("draw_obj_with_id=" + std::to_string(obj.id), obj.draw_order,
            [obj](rocket::renderer_2d *ren, rocket::window_backend_i *win) {
                if (!obj.texture) ren->draw_rectangle(obj.position, obj.size, obj.color, obj.rotation);
            });

        return game_objects.size() - 1;
    };

    objects_layout->addStretch();

    QPushButton *new_object_button = new QPushButton("Create New Object");
    QObject::connect(new_object_button, &QPushButton::clicked, [=]() { register_obj(); });
    inspector_layout->addWidget(new_object_button);
    inspector_layout->addWidget(objects_scroll, 1);

    // -------------------------
    // load_scene helper
    // -------------------------
    auto load_scene = [&](const QString &scene_path) {
        std::ifstream file(scene_path.toStdString());
        if (!file.is_open()) return;

        nlohmann::json j;
        try { file >> j; } catch (...) {
            QMessageBox::critical(&window, "Error", "Failed to parse scene file.");
            return;
        }

        for (auto &obj : game_objects)
            viewport->remove_drawcall("draw_obj_with_id=" + std::to_string(obj.id));
        game_objects.clear();
        history.undo_stack.clear();
        history.redo_stack.clear();
        btn_undo->setEnabled(false);
        btn_redo->setEnabled(false);

        QLayoutItem *item;
        while ((item = objects_layout->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        objects_layout->addStretch();

        global_id = j.value("global_id", 0);

        for (auto &o : j["objects"]) {
            game_object_t preset;
            preset.id         = o["id"].get<int>();
            preset.name       = o["name"].get<std::string>();
            preset.position   = { o["position"][0].get<float>(), o["position"][1].get<float>() };
            preset.size       = { o["size"][0].get<float>(),     o["size"][1].get<float>() };
            preset.color      = { (uint8_t)o["color"][0].get<int>(), (uint8_t)o["color"][1].get<int>(),
                                  (uint8_t)o["color"][2].get<int>(), (uint8_t)o["color"][3].get<int>() };
            preset.rotation   = o["rotation"].get<float>();
            preset.draw_order = o["draw_order"].get<int>();
            std::string tex_path = o["texture"].get<std::string>();
            if (!tex_path.empty())
                preset.texture = am->get_texture(am->load_texture(tex_path));
            register_obj(preset);
        }

        current_scene_path = scene_path;
    };

    // -------------------------
    // Menu page buttons
    // -------------------------
    QObject::connect(new_button, &QPushButton::clicked, [&]() {
        QString dir = QFileDialog::getExistingDirectory(
            &window, "Select Project Folder", "",
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (dir.isEmpty()) return;

        QDir project_dir(dir);
        project_dir.mkpath("scenes");
        project_dir.mkpath("assets/textures");
        project_dir.mkpath("assets/audio");
        project_dir.mkpath("assets/fonts");

        QString project_file = dir + "/RGProject.json";
        if (!QFile::exists(project_file)) {
            nlohmann::json j;
            j["name"]           = project_dir.dirName().toStdString();
            j["version"]        = "1.0.0";
            j["engine_version"] = ROCKETGE__VERSION;
            j["default_scene"]  = "scenes/Scene1.rgscene";
            j["scenes"]         = nlohmann::json::array();
            std::ofstream f(project_file.toStdString());
            f << j.dump(4);
        }

        current_project_dir = dir;
        current_scene_path  = dir + "/scenes/Scene1.rgscene";
        window.setWindowTitle("RocketGE - Editor | " + project_dir.dirName());
        stack->setCurrentWidget(editor_page);
    });

    QObject::connect(_Debug_new, &QPushButton::clicked, [&]() {
        QString dir = "/home/noerlol/Documents/RocketGE";
        if (dir.isEmpty()) return;

        QString project_file = dir + "/RGProject.json";
        if (!QFile::exists(project_file)) {
            QMessageBox::critical(&window, "Error",
                "No RGProject.json found.\nThis doesn't appear to be a RocketGE project.");
            return;
        }

        std::ifstream f(project_file.toStdString());
        nlohmann::json j;
        try { f >> j; } catch (...) {
            QMessageBox::critical(&window, "Error", "Failed to parse RGProject.json.");
            return;
        }

        current_project_dir = dir;
        QDir project_dir(dir);
        window.setWindowTitle("RocketGE - Editor | " + project_dir.dirName());

        std::string default_scene = j.value("default_scene", "");
        if (!default_scene.empty()) {
            QString scene_path = dir + "/" + QString::fromStdString(default_scene);
            if (QFile::exists(scene_path))
                load_scene(scene_path);
        }

        stack->setCurrentWidget(editor_page);
    });

    QObject::connect(open_button, &QPushButton::clicked, [&]() {
        QString dir = QFileDialog::getExistingDirectory(
            &window, "Open Project Folder", "",
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (dir.isEmpty()) return;

        QString project_file = dir + "/RGProject.json";
        if (!QFile::exists(project_file)) {
            QMessageBox::critical(&window, "Error",
                "No RGProject.json found.\nThis doesn't appear to be a RocketGE project.");
            return;
        }

        std::ifstream f(project_file.toStdString());
        nlohmann::json j;
        try { f >> j; } catch (...) {
            QMessageBox::critical(&window, "Error", "Failed to parse RGProject.json.");
            return;
        }

        current_project_dir = dir;
        QDir project_dir(dir);
        window.setWindowTitle("RocketGE - Editor | " + project_dir.dirName());

        std::string default_scene = j.value("default_scene", "");
        if (!default_scene.empty()) {
            QString scene_path = dir + "/" + QString::fromStdString(default_scene);
            if (QFile::exists(scene_path))
                load_scene(scene_path);
        }

        stack->setCurrentWidget(editor_page);
    });

    // -------------------------
    // Toolbar buttons
    // -------------------------
    QObject::connect(btn_new, &QToolButton::clicked, [&]() {
        for (auto &obj : game_objects)
            viewport->remove_drawcall("draw_obj_with_id=" + std::to_string(obj.id));
        game_objects.clear();
        global_id = 0;
        current_scene_path = "";
        history.undo_stack.clear();
        history.redo_stack.clear();
        btn_undo->setEnabled(false);
        btn_redo->setEnabled(false);
        QLayoutItem *item;
        while ((item = objects_layout->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        objects_layout->addStretch();
    });

    QObject::connect(btn_open, &QToolButton::clicked, [&]() {
        QString path = QFileDialog::getOpenFileName(
            &window, "Open Scene", "", "RocketGE Scene (*.rgscene);;JSON (*.json)");
        if (path.isEmpty()) return;
        load_scene(path);
        stack->setCurrentWidget(editor_page);
    });

    QObject::connect(btn_save, &QToolButton::clicked, [&]() {
        if (current_scene_path.isEmpty()) {
            current_scene_path = QFileDialog::getSaveFileName(
                &window, "Save Scene", "", "RocketGE Scene (*.rgscene);;JSON (*.json)");
            if (current_scene_path.isEmpty()) return;
        }

        nlohmann::json j;
        j["global_id"] = global_id;
        j["objects"]   = nlohmann::json::array();

        for (auto &obj : game_objects) {
            nlohmann::json o;
            o["id"]         = obj.id;
            o["name"]       = obj.name;
            o["position"]   = { obj.position.x, obj.position.y };
            o["size"]       = { obj.size.x,      obj.size.y };
            o["color"]      = { obj.color.x,     obj.color.y, obj.color.z, obj.color.w };
            o["rotation"]   = obj.rotation;
            o["draw_order"] = obj.draw_order;
            o["texture"]    = obj.texture ? obj.texture->path : "";
            j["objects"].push_back(o);
        }

        std::ofstream file(current_scene_path.toStdString());
        if (!file.is_open()) {
            QMessageBox::critical(&window, "Error", "Failed to save scene.");
            return;
        }
        file << j.dump(4);

        // Update RGProject.json scene list
        if (!current_project_dir.isEmpty()) {
            QString project_file = current_project_dir + "/RGProject.json";
            std::ifstream pf(project_file.toStdString());
            if (pf.is_open()) {
                nlohmann::json pj;
                try {
                    pf >> pj; pf.close();
                    std::string rel = QDir(current_project_dir)
                        .relativeFilePath(current_scene_path).toStdString();
                    auto &scenes = pj["scenes"];
                    if (std::find(scenes.begin(), scenes.end(), rel) == scenes.end())
                        scenes.push_back(rel);
                    std::ofstream out(project_file.toStdString());
                    out << pj.dump(4);
                } catch (...) {}
            }
        }

        rocket::log("Scene saved: " + current_scene_path.toStdString(),
            "editor", "btn_save", "info");
    });

    QObject::connect(btn_undo, &QToolButton::clicked, [&]() {
        history.undo();
        btn_undo->setEnabled(history.can_undo());
        btn_redo->setEnabled(history.can_redo());
    });

    QObject::connect(btn_redo, &QToolButton::clicked, [&]() {
        history.redo();
        btn_undo->setEnabled(history.can_undo());
        btn_redo->setEnabled(history.can_redo());
    });

    QShortcut *undo_shortcut = new QShortcut(QKeySequence::Undo, &window);
    QObject::connect(undo_shortcut, &QShortcut::activated, [&]() {
        history.undo();
        btn_undo->setEnabled(history.can_undo());
        btn_redo->setEnabled(history.can_redo());
    });

    QShortcut *redo_shortcut = new QShortcut(QKeySequence::Redo, &window);
    QObject::connect(redo_shortcut, &QShortcut::activated, [&]() {
        history.redo();
        btn_undo->setEnabled(history.can_undo());
        btn_redo->setEnabled(history.can_redo());
    });

    QShortcut *save_shortcut = new QShortcut(QKeySequence::Save, &window);
    QObject::connect(save_shortcut, &QShortcut::activated, [&]() {
        btn_save->click();
    });

    QObject::connect(btn_pause, &QToolButton::clicked, [&](bool checked) {
        viewport->paused = checked;
    });

    QObject::connect(btn_codegen, &QToolButton::clicked, [&]() {
        rgeditor::generated_code_t *code = rgeditor::gen();

        QDialog *dialog = new QDialog(&window);
        dialog->setWindowTitle("Generated Code");
        dialog->resize(800, 600);

        QVBoxLayout *layout = new QVBoxLayout(dialog);

        QTabWidget *tabs = new QTabWidget;

        auto make_tab = [](const QString &content, const QString &title) -> QWidget* {
            QWidget *tab = new QWidget;
            QVBoxLayout *l = new QVBoxLayout(tab);
            l->setContentsMargins(0, 0, 0, 0);

            QPlainTextEdit *edit = new QPlainTextEdit(content);
            edit->setReadOnly(true);
            edit->setFont(QFont("Monospace", 10));
            edit->setStyleSheet("background-color: rgb(30,30,30); color: #ddd;");

            QPushButton *copy_btn = new QPushButton("Copy to Clipboard");
            copy_btn->setStyleSheet("min-height: 28px;");
            QObject::connect(copy_btn, &QPushButton::clicked, [edit]() {
                QApplication::clipboard()->setText(edit->toPlainText());
            });

            l->addWidget(edit, 1);
            l->addWidget(copy_btn);
            return tab;
        };

        tabs->addTab(make_tab(QString::fromStdString(code->hpp), "Header"), "rge_generated_code.hpp");
        tabs->addTab(make_tab(QString::fromStdString(code->cpp), "Source"), "main.cpp");

        QPushButton *close_btn = new QPushButton("Close");
        close_btn->setStyleSheet("min-height: 28px;");
        QObject::connect(close_btn, &QPushButton::clicked, dialog, &QDialog::accept);

        layout->addWidget(tabs, 1);
        layout->addWidget(close_btn);

        dialog->exec();
    });

    btn_undo->setEnabled(false);
    btn_redo->setEnabled(false);

    window.resize(1600, 900);
    window.show();

    return app.exec();
}

#include "main.moc"
