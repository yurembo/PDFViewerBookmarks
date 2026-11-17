#include "bookmark_store.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <utility>

namespace {

	QString sidecarPathFor(const QString& pdfPath) {
		if (pdfPath.isEmpty())
			return QString();

		const QFileInfo info(pdfPath);
		return info.dir().filePath(info.completeBaseName() + QStringLiteral(".bookmarks.json"));
	}

}  // namespace

BookmarkStore::BookmarkStore(const QString& pdfPath)
	: m_sidecarPath(sidecarPathFor(pdfPath)) {
	if (m_sidecarPath.isEmpty())
		return;

	QFile file(m_sidecarPath);
	if (!file.open(QIODevice::ReadOnly))
		return;

	const QJsonArray array = QJsonDocument::fromJson(file.readAll()).array();
	m_bookmarks.reserve(array.size());
	for (const QJsonValue& value : array) {
		const QJsonObject object = value.toObject();
		Bookmark bookmark;
		bookmark.page = object.value(QStringLiteral("page")).toInt();
		bookmark.title = object.value(QStringLiteral("title")).toString();
		if (!bookmark.title.isEmpty())
			m_bookmarks.append(bookmark);
	}
}

void BookmarkStore::setBookmarks(QList<Bookmark> bookmarks) {
	m_bookmarks = std::move(bookmarks);
}

bool BookmarkStore::save() const {
	if (m_sidecarPath.isEmpty())
		return false;

	QJsonArray array;
	for (const Bookmark& bookmark : m_bookmarks) {
		QJsonObject object;
		object.insert(QStringLiteral("page"), bookmark.page);
		object.insert(QStringLiteral("title"), bookmark.title);
		array.append(object);
	}

	QFile file(m_sidecarPath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return false;
	return file.write(QJsonDocument(array).toJson(QJsonDocument::Indented)) >= 0;
}
