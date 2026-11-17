#pragma once

#include <QList>
#include <QString>

struct Bookmark {
    int page = 0;
    QString title;
};

// Хранит закладки одного PDF-документа в JSON-файле рядом с самим
// документом ("<имя_pdf>.bookmarks.json"), чтобы они переезжали вместе
// с файлом и не требовали никакой внешней базы.
class BookmarkStore {
public:
    explicit BookmarkStore(const QString &pdfPath);

    const QList<Bookmark> &bookmarks() const { return m_bookmarks; }
    void setBookmarks(QList<Bookmark> bookmarks);
    bool save() const;
    bool isValid() const { return !m_sidecarPath.isEmpty(); }

private:
    QString m_sidecarPath;
    QList<Bookmark> m_bookmarks;
};
