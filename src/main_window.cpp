#include "main_window.h"

#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QInputDialog>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPoint>
#include <QStatusBar>
#include <QToolBar>

#include <QPdfDocument>
#include <QPdfPageNavigator>
#include <QPdfView>

#include <memory>
#include <utility>

namespace {

	constexpr qreal kZoomStep = 1.25;

}  // namespace

MainWindow::MainWindow() {
	createUi();
	createActions();
	setWindowTitle(tr("Просмотр PDF"));
	resize(1100, 750);
}

void MainWindow::createUi() {
	m_document = new QPdfDocument(this);

	m_pdfView = new QPdfView(this);
	m_pdfView->setDocument(m_document);
	m_pdfView->setPageMode(QPdfView::PageMode::MultiPage);
	m_pdfView->setZoomMode(QPdfView::ZoomMode::FitToWidth);
	setCentralWidget(m_pdfView);

	auto* bookmarkDock = new QDockWidget(tr("Закладки"), this);
	bookmarkDock->setObjectName(QStringLiteral("bookmarkDock"));
	bookmarkDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	m_bookmarkList = new QListWidget(bookmarkDock);
	m_bookmarkList->setAlternatingRowColors(true);
	bookmarkDock->setWidget(m_bookmarkList);
	addDockWidget(Qt::LeftDockWidgetArea, bookmarkDock);

	m_pageLabel = new QLabel(this);
	statusBar()->addPermanentWidget(m_pageLabel);

	connect(m_pdfView->pageNavigator(), &QPdfPageNavigator::currentPageChanged,
		this, &MainWindow::updatePageLabel);
	connect(m_bookmarkList, &QListWidget::itemActivated,
		this, &MainWindow::goToBookmark);
	connect(m_bookmarkList, &QListWidget::itemClicked,
		this, &MainWindow::goToBookmark);

	updatePageLabel();
}

void MainWindow::createActions() {
	QMenu* fileMenu = menuBar()->addMenu(tr("&Файл"));

	QToolBar* toolBar = addToolBar(tr("Основная"));
	toolBar->setObjectName(QStringLiteral("mainToolBar"));
	toolBar->setMovable(false);
	toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

	QAction* openAction = fileMenu->addAction(QIcon::fromTheme(QStringLiteral("document-open")),
		tr("&Открыть…"), this, &MainWindow::openFileViaDialog);
	openAction->setShortcut(QKeySequence::Open);
	toolBar->addAction(openAction);

	m_addBookmarkAction = fileMenu->addAction(QIcon::fromTheme(QStringLiteral("list-add")),
		tr("Добавить &закладку…"), this, &MainWindow::addBookmark);
	m_addBookmarkAction->setShortcut(QKeySequence(tr("Ctrl+B")));
	m_addBookmarkAction->setEnabled(false);
	toolBar->addAction(m_addBookmarkAction);

	m_removeBookmarkAction = fileMenu->addAction(QIcon::fromTheme(QStringLiteral("list-remove")),
		tr("&Удалить закладку"), this, &MainWindow::removeSelectedBookmark);
	m_removeBookmarkAction->setShortcut(QKeySequence::Delete);
	m_removeBookmarkAction->setEnabled(false);
	toolBar->addAction(m_removeBookmarkAction);

	fileMenu->addSeparator();
	QAction* quitAction = fileMenu->addAction(tr("В&ыход"));
	connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
	quitAction->setShortcut(QKeySequence::Quit);

	QMenu* viewMenu = menuBar()->addMenu(tr("&Вид"));
	QAction* zoomInAction = viewMenu->addAction(tr("&Увеличить"), this, [this] {
		m_pdfView->setZoomFactor(m_pdfView->zoomFactor() * kZoomStep);
		});
	zoomInAction->setShortcut(QKeySequence::ZoomIn);

	QAction* zoomOutAction = viewMenu->addAction(tr("У&меньшить"), this, [this] {
		m_pdfView->setZoomFactor(m_pdfView->zoomFactor() / kZoomStep);
		});
	zoomOutAction->setShortcut(QKeySequence::ZoomOut);

	viewMenu->addSeparator();
	viewMenu->addAction(tr("По &ширине"), this, [this] {
		m_pdfView->setZoomMode(QPdfView::ZoomMode::FitToWidth);
		})->setShortcut(QKeySequence(tr("Ctrl+1")));

	viewMenu->addAction(tr("Страница &целиком"), this, [this] {
		m_pdfView->setZoomMode(QPdfView::ZoomMode::FitInView);
		})->setShortcut(QKeySequence(tr("Ctrl+2")));
}

void MainWindow::openFile(const QString& path) {
	if (path.isEmpty())
		return;

	QPdfDocument::Error error = m_document->load(path);
	if (error == QPdfDocument::Error::IncorrectPassword) {
		bool ok = false;
		const QString password = QInputDialog::getText(this, tr("Пароль"),
			tr("Документ защищён паролем:"), QLineEdit::Password, QString(), &ok);
		if (ok) {
			m_document->setPassword(password);
			error = m_document->load(path);
		}
	}
	if (error != QPdfDocument::Error::None) {
		QMessageBox::warning(this, tr("Открытие файла"),
			tr("Не удалось открыть PDF-файл:\n%1").arg(path));
		return;
	}

	m_bookmarkList->clear();
	m_store = std::make_unique<BookmarkStore>(path);
	m_addBookmarkAction->setEnabled(true);
	reloadBookmarkList();

	setWindowTitle(tr("%1 — Просмотр PDF").arg(QFileInfo(path).fileName()));
	updatePageLabel();
}

void MainWindow::openFileViaDialog() {
	const QString path = QFileDialog::getOpenFileName(this, tr("Открыть PDF-файл"),
		QString(), tr("PDF-документы (*.pdf);;Все файлы (*)"));
	if (!path.isEmpty())
		openFile(path);
}

void MainWindow::addBookmark() {
	if (!m_store)
		return;

	const int page = m_pdfView->pageNavigator()->currentPage();
	bool ok = false;
	QString title = QInputDialog::getText(this, tr("Новая закладка"),
		tr("Название для страницы %1:").arg(page + 1), QLineEdit::Normal,
		tr("Страница %1").arg(page + 1), &ok);
	if (!ok)
		return;
	title = title.trimmed();
	if (title.isEmpty())
		return;

	QList<Bookmark> bookmarks = m_store->bookmarks();
	Bookmark bookmark;
	bookmark.page = page;
	bookmark.title = title;
	bookmarks.append(bookmark);
	m_store->setBookmarks(std::move(bookmarks));
	if (!m_store->save())
		statusBar()->showMessage(tr("Не удалось сохранить закладки"), 4000);
	reloadBookmarkList();
}

void MainWindow::removeSelectedBookmark() {
	if (!m_store)
		return;

	const int row = m_bookmarkList->currentRow();
	QList<Bookmark> bookmarks = m_store->bookmarks();
	if (row < 0 || row >= bookmarks.size())
		return;

	bookmarks.removeAt(row);
	m_store->setBookmarks(std::move(bookmarks));
	if (!m_store->save())
		statusBar()->showMessage(tr("Не удалось сохранить закладки"), 4000);
	reloadBookmarkList();
}

void MainWindow::goToBookmark(QListWidgetItem* item) {
	if (!item)
		return;
	m_pdfView->pageNavigator()->jump(item->data(Qt::UserRole).toInt(), QPointF(0, 0));
}

void MainWindow::reloadBookmarkList() {
	m_bookmarkList->clear();
	if (!m_store)
		return;

	const QList<Bookmark> bookmarks = m_store->bookmarks();
	for (const Bookmark& bookmark : bookmarks) {
		auto* item = new QListWidgetItem(
			tr("Стр. %1 — %2").arg(bookmark.page + 1).arg(bookmark.title), m_bookmarkList);
		item->setData(Qt::UserRole, bookmark.page);
		item->setToolTip(bookmark.title);
	}
	m_removeBookmarkAction->setEnabled(!bookmarks.isEmpty());
}

void MainWindow::updatePageLabel() {
	const int pageCount = m_document->pageCount();
	const int currentPage = m_pdfView->pageNavigator()->currentPage();
	if (pageCount <= 0) {
		m_pageLabel->setText(tr("Документ не открыт"));
		return;
	}
	m_pageLabel->setText(tr("Страница %1 из %2").arg(currentPage + 1).arg(pageCount));
}
