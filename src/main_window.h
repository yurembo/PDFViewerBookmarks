#pragma once

#include <QList>
#include <QListWidgetItem>
#include <QMainWindow>
#include <QString>

#include <memory>

#include "bookmark_store.h"

class QAction;
class QLabel;
class QPdfDocument;
class QPdfView;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow();

    void openFile(const QString &path);

private slots:
    void openFileViaDialog();
    void addBookmark();
    void removeSelectedBookmark();
    void goToBookmark(QListWidgetItem *item);
    void updatePageLabel();

private:
    void createUi();
    void createActions();
    void reloadBookmarkList();

    QPdfDocument *m_document = nullptr;
    QPdfView *m_pdfView = nullptr;
    QListWidget *m_bookmarkList = nullptr;
    QLabel *m_pageLabel = nullptr;
    QAction *m_addBookmarkAction = nullptr;
    QAction *m_removeBookmarkAction = nullptr;
    std::unique_ptr<BookmarkStore> m_store;
};
