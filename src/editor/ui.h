/********************************************************************************
** Form generated from reading UI file 'RGEditorYdjDkL.ui'
**
** Created by: Qt User Interface Compiler version 6.10.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef RGEDITORYDJDKL_H
#define RGEDITORYDJDKL_H

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
#include "collapsiblesection.h"

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QHBoxLayout *horizontalLayout_2;
    QStackedWidget *stackedWidget;
    QWidget *main_menu;
    QHBoxLayout *horizontalLayout_6;
    QVBoxLayout *verticalLayout_3;
    QLabel *label;
    QPushButton *pushButton_2;
    QPushButton *pushButton;
    QSpacerItem *verticalSpacer;
    QLabel *label_2;
    QVBoxLayout *verticalLayout_4;
    QListWidget *listWidget;
    QWidget *new_project;
    QWidget *editor;
    QHBoxLayout *horizontalLayout_4;
    QHBoxLayout *horizontalLayout_5;
    QTabWidget *game_tab_2;
    QWidget *game_tab;
    QHBoxLayout *horizontalLayout_7;
    QOpenGLWidget *openGLWidget;
    QWidget *tab_objects;
    QHBoxLayout *horizontalLayout_8;
    QScrollArea *scrollArea;
    QWidget *scrollAreaWidgetContents;
    QHBoxLayout *horizontalLayout_9;
    QVBoxLayout *verticalLayout_6;
    CollapsibleSection *widget;
    QSpacerItem *verticalSpacer_2;
    QSpacerItem *horizontalSpacer;
    QScrollBar *verticalScrollBar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1116, 800);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        horizontalLayout_2 = new QHBoxLayout(centralwidget);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        stackedWidget = new QStackedWidget(centralwidget);
        stackedWidget->setObjectName("stackedWidget");
        main_menu = new QWidget();
        main_menu->setObjectName("main_menu");
        horizontalLayout_6 = new QHBoxLayout(main_menu);
        horizontalLayout_6->setObjectName("horizontalLayout_6");
        verticalLayout_3 = new QVBoxLayout();
        verticalLayout_3->setSpacing(0);
        verticalLayout_3->setObjectName("verticalLayout_3");
        verticalLayout_3->setSizeConstraint(QLayout::SizeConstraint::SetFixedSize);
        verticalLayout_3->setContentsMargins(0, -1, -1, 0);
        label = new QLabel(main_menu);
        label->setObjectName("label");

        verticalLayout_3->addWidget(label);

        pushButton_2 = new QPushButton(main_menu);
        pushButton_2->setObjectName("pushButton_2");
        pushButton_2->setMinimumSize(QSize(0, 34));

        verticalLayout_3->addWidget(pushButton_2);

        pushButton = new QPushButton(main_menu);
        pushButton->setObjectName("pushButton");
        pushButton->setEnabled(true);
        QSizePolicy sizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(pushButton->sizePolicy().hasHeightForWidth());
        pushButton->setSizePolicy(sizePolicy);

        verticalLayout_3->addWidget(pushButton);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_3->addItem(verticalSpacer);

        label_2 = new QLabel(main_menu);
        label_2->setObjectName("label_2");

        verticalLayout_3->addWidget(label_2);


        horizontalLayout_6->addLayout(verticalLayout_3);

        verticalLayout_4 = new QVBoxLayout();
        verticalLayout_4->setObjectName("verticalLayout_4");
        listWidget = new QListWidget(main_menu);
        new QListWidgetItem(listWidget);
        new QListWidgetItem(listWidget);
        new QListWidgetItem(listWidget);
        new QListWidgetItem(listWidget);
        new QListWidgetItem(listWidget);
        listWidget->setObjectName("listWidget");
        listWidget->setStyleSheet(QString::fromUtf8("    QListWidget {\n"
"        background-color: rgb(30, 30, 30);\n"
"        border: 1px solid rgb(50, 50, 50);\n"
"        padding: 2px;\n"
"    }\n"
"\n"
"    QListWidget::item {\n"
"        color: white;\n"
"        background-color: rgb(40, 40, 40);\n"
"        border: 1px solid rgb(60, 60, 60);\n"
"        margin: 2px;\n"
"        padding: 5px;\n"
"        border-radius: 4px;\n"
"    }\n"
"\n"
"    QListWidget::item:selected {\n"
"        background-color: rgb(70, 70, 150);\n"
"        color: white;\n"
"    }\n"
"\n"
"    QListWidget::item:hover {\n"
"        background-color: rgb(60, 60, 100);\n"
"    }\n"
"\n"
"    QScrollBar:vertical {\n"
"        background: rgb(35, 35, 35);\n"
"        width: 10px;\n"
"        margin: 0px 0px 0px 0px;\n"
"    }\n"
"\n"
"    QScrollBar::handle:vertical {\n"
"        background: rgb(80, 80, 80);\n"
"        min-height: 20px;\n"
"        border-radius: 5px;\n"
"    }\n"
"\n"
"    QScrollBar::add-line, QScrollBar::sub-line {\n"
"        height: 0px; /* hide arrows */\n"
""
                        "    }"));

        verticalLayout_4->addWidget(listWidget);


        horizontalLayout_6->addLayout(verticalLayout_4);

        stackedWidget->addWidget(main_menu);
        new_project = new QWidget();
        new_project->setObjectName("new_project");
        stackedWidget->addWidget(new_project);
        editor = new QWidget();
        editor->setObjectName("editor");
        horizontalLayout_4 = new QHBoxLayout(editor);
        horizontalLayout_4->setObjectName("horizontalLayout_4");
        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName("horizontalLayout_5");
        game_tab_2 = new QTabWidget(editor);
        game_tab_2->setObjectName("game_tab_2");
        game_tab = new QWidget();
        game_tab->setObjectName("game_tab");
        horizontalLayout_7 = new QHBoxLayout(game_tab);
        horizontalLayout_7->setObjectName("horizontalLayout_7");
        // openGLWidget = new QOpenGLWidget(game_tab);
        // openGLWidget->setObjectName("openGLWidget");

        // horizontalLayout_7->addWidget(openGLWidget);

        game_tab_2->addTab(game_tab, QString());
        tab_objects = new QWidget();
        tab_objects->setObjectName("tab_objects");
        horizontalLayout_8 = new QHBoxLayout(tab_objects);
        horizontalLayout_8->setObjectName("horizontalLayout_8");
        scrollArea = new QScrollArea(tab_objects);
        scrollArea->setObjectName("scrollArea");
        scrollArea->setWidgetResizable(true);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName("scrollAreaWidgetContents");
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 1072, 727));
        horizontalLayout_9 = new QHBoxLayout(scrollAreaWidgetContents);
        horizontalLayout_9->setObjectName("horizontalLayout_9");
        verticalLayout_6 = new QVBoxLayout();
        verticalLayout_6->setObjectName("verticalLayout_6");
        widget = new CollapsibleSection(scrollAreaWidgetContents);
        widget->setObjectName("widget");

        verticalLayout_6->addWidget(widget);

        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        verticalLayout_6->addItem(verticalSpacer_2);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        verticalLayout_6->addItem(horizontalSpacer);


        horizontalLayout_9->addLayout(verticalLayout_6);

        verticalScrollBar = new QScrollBar(scrollAreaWidgetContents);
        verticalScrollBar->setObjectName("verticalScrollBar");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Maximum, QSizePolicy::Policy::Minimum);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(verticalScrollBar->sizePolicy().hasHeightForWidth());
        verticalScrollBar->setSizePolicy(sizePolicy1);
        verticalScrollBar->setMaximum(99);
        verticalScrollBar->setOrientation(Qt::Orientation::Vertical);

        horizontalLayout_9->addWidget(verticalScrollBar);

        scrollArea->setWidget(scrollAreaWidgetContents);

        horizontalLayout_8->addWidget(scrollArea);

        game_tab_2->addTab(tab_objects, QString());

        horizontalLayout_5->addWidget(game_tab_2);


        horizontalLayout_4->addLayout(horizontalLayout_5);

        stackedWidget->addWidget(editor);

        horizontalLayout_2->addWidget(stackedWidget);

        MainWindow->setCentralWidget(centralwidget);

        retranslateUi(MainWindow);
        QObject::connect(pushButton, &QPushButton::clicked, stackedWidget, qOverload<>(&QStackedWidget::setFocus));

        stackedWidget->setCurrentIndex(2);
        game_tab_2->setCurrentIndex(1);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "<html><head/><body><p align=\"center\"><span style=\" font-size:18pt;\">RocketGE</span></p></body></html>", nullptr));
        pushButton_2->setText(QCoreApplication::translate("MainWindow", "Create New Project", nullptr));
        pushButton->setText(QCoreApplication::translate("MainWindow", "Open Existing Project", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "Version: PLACEHOLDER", nullptr));

        const bool __sortingEnabled = listWidget->isSortingEnabled();
        listWidget->setSortingEnabled(false);
        QListWidgetItem *___qlistwidgetitem = listWidget->item(0);
        ___qlistwidgetitem->setText(QCoreApplication::translate("MainWindow", "Snake", nullptr));
        QListWidgetItem *___qlistwidgetitem1 = listWidget->item(1);
        ___qlistwidgetitem1->setText(QCoreApplication::translate("MainWindow", "Pong", nullptr));
        QListWidgetItem *___qlistwidgetitem2 = listWidget->item(2);
        ___qlistwidgetitem2->setText(QCoreApplication::translate("MainWindow", "Tetris", nullptr));
        QListWidgetItem *___qlistwidgetitem3 = listWidget->item(3);
        ___qlistwidgetitem3->setText(QCoreApplication::translate("MainWindow", "JustAnotherTest", nullptr));
        QListWidgetItem *___qlistwidgetitem4 = listWidget->item(4);
        ___qlistwidgetitem4->setText(QCoreApplication::translate("MainWindow", "JustAPlaceholder", nullptr));
        listWidget->setSortingEnabled(__sortingEnabled);

        game_tab_2->setTabText(game_tab_2->indexOf(game_tab), QCoreApplication::translate("MainWindow", "Game", nullptr));
        game_tab_2->setTabText(game_tab_2->indexOf(tab_objects), QCoreApplication::translate("MainWindow", "Objects", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // RGEDITORYDJDKL_H
