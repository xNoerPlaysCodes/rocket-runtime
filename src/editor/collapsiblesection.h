#ifndef RGEDITOR__COLLAPSIBLE_SECTION
#define RGEDITOR__COLLAPSIBLE_SECTION

#include <QtCore/QVariant>
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <QToolButton>

class CollapsibleSection : public QWidget {
public:
    CollapsibleSection(QWidget* content, QWidget* parent = nullptr)
        : QWidget(parent)
    {
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0,0,0,0);

        QToolButton* header = new QToolButton;
        header->setText("UnknownCollapsibleSection");
        header->setCheckable(true);
        header->setChecked(true);
        header->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        header->setArrowType(Qt::DownArrow);

        layout->addWidget(header);
        layout->addWidget(content);

        QObject::connect(header, &QToolButton::toggled, [=](bool open){
            content->setVisible(open);
            header->setArrowType(open ? Qt::DownArrow : Qt::RightArrow);
        });
    }
};

#endif//RGEDITOR__COLLAPSIBLE_SECTION
