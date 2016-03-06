#ifndef DBQUERYWIDGET_H
#define DBQUERYWIDGET_H

#include <QWidget>

#include <memory>

QT_FORWARD_DECLARE_CLASS(QItemSelection)

namespace Settings {
class KnownFieldModel;
}

namespace QtGui {

namespace Ui {
class DbQueryWidget;
}

class QueryResultsModel;
class TagEditorWidget;
class TagEdit;

class DbQueryWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DbQueryWidget(TagEditorWidget *tagEditorWidget, QWidget *parent = nullptr);
    ~DbQueryWidget();

    void insertSearchTermsFromTagEdit(TagEdit *tagEdit);

public slots:
    void startSearch();
    void abortSearch();

private slots:
    void showResults();
    void setStatus(bool aborted);
    void fileStatusChanged(bool opened, bool hasTags);
    void applyResults();
    void showResultsContextMenu();
    void fetchAndShowCoverForSelection();
    void showCover(const QByteArray &data);
    void showCoverFromIndex(const QModelIndex &index);

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private:
    std::unique_ptr<Ui::DbQueryWidget> m_ui;
    TagEditorWidget *m_tagEditorWidget;
    QueryResultsModel *m_model;
    int m_coverIndex;
};

}

#endif // DBQUERYWIDGET_H
