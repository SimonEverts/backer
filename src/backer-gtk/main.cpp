#include "katla/core/core-application.h"

#include "katla/gtk/gtk-application.h"

#include "katla/gtk/gtk-text-impl.h"
#include "katla/gtk/gtk-button-impl.h"
#include "katla/gtk/gtk-column.h"
#include "katla/gtk/gtk-row.h"
#include "katla/gtk/gtk-list-view.h"

#include "libbacker/file-data.h"
#include "libbacker/file-group-set.h"

#include "string.h"
#include <iostream>
#include <vector>

int main(int argc, char* argv[])
{
    std::vector<std::string> args;
    for (int i=0; i<argc; i++) {
        std::string arg = argv[i];
        args.push_back(arg);
    }

    katla::GtkApplication gtkApplication;
    gtkApplication.init(argc, argv, "Backer");

    std::shared_ptr<katla::GtkWindowImpl> window;

    auto listview = std::make_shared<katla::GtkListView>();
    auto duplicateListView = std::make_shared<katla::GtkListView>();

    auto genListEntry = [](const backer::FileSystemEntry& file) {
        auto entryLabel = std::make_shared<katla::GtkTextImpl>();
        entryLabel->updateText({katla::format("{}", file.absolutePath)});
        entryLabel->init();

//        auto entryButton = std::make_shared<katla::GtkButtonImpl>();
//        entryButton->updateButton({katla::format("{} Button", text)});
//        entryButton->init();
//        entryButton->onClicked([text]() {
//            katla::printInfo(katla::format("Clicked on: {}", text));
//        });

        auto row = std::make_shared<katla::GtkRow>();
        row->updateContainer({
                                     .children = {
                                             katla::ContainerChild{.child = entryLabel, true, true, 16},
//                                             katla::ContainerChild{.child = entryButton, false, true, 0}
                                     }
                             });
        row->init();
        row->show();
        return row;
    };


    gtkApplication.onStartup([&gtkApplication, &window, &listview, &duplicateListView, &genListEntry]() {
        katla::printInfo("startup!");

        window = gtkApplication.createWindow("Backer", {800,600});

        auto label = std::make_shared<katla::GtkTextImpl>();
        label->updateText({"File list:"});
        label->init();

        listview = std::make_shared<katla::GtkListView>();
        listview->updateContainer({
            .children = {}
        });

        listview->init();
        listview->show();

        duplicateListView = std::make_shared<katla::GtkListView>();
        duplicateListView->updateContainer({.children = {}});

        duplicateListView->init();
        duplicateListView->show();

        auto column = std::make_shared<katla::GtkColumn>();
        column->init();

        column->updateContainer({
                .children = {
                        katla::ContainerChild{.child = label},
                        katla::ContainerChild{.child = listview, .expand = true, .fill = true},
                        katla::ContainerChild{.child = duplicateListView, .expand = false, .fill = true, .height = 200}
                }
        });

        window->updateContainer({
                .children = {katla::ContainerChild{.child = std::static_pointer_cast<katla::Widget>(column)}}
        });
    });

    backer::FileGroupSet fileGroupSet;

//    katla::Subject<std::string> onListActivated;

    std::map<std::string, std::vector<backer::FileSystemEntry>> fileMap;
    std::vector<std::string> keys;

    gtkApplication.onActivate([&fileMap, &keys, &window, &listview, &genListEntry, &fileGroupSet, &duplicateListView]() {
        window->show();

        fileGroupSet = backer::FileGroupSet::create("/mnt/exthdd/primary"); // /mnt/exthdd/primary
        fileMap = fileGroupSet.fileMap();

        for (auto& pair : fileMap) {
            if (pair.second.size() < 2) {
                continue;
            }

            auto front = pair.second.front();
            if (front.type != backer::FileSystemEntryType::Dir) {
                continue;
            }

            keys.push_back(pair.first);
//            for(auto& file : pair.second) {
                listview->addWidget(genListEntry(pair.second.front()));
//            }
        }

        listview->onRowSelected([&fileMap, &keys, &duplicateListView, &genListEntry](int index) {
            katla::printInfo("Row selected {}", index);

            duplicateListView->clear();
            for (auto& file : fileMap[keys[index]]) {
                duplicateListView->addWidget(genListEntry(file));
            }
        });

    });

    return gtkApplication.run();
}
