#ifndef TABSTYLE_H
#define TABSTYLE_H
#include <QtGui>
#include <QtCore>
#include <QtWidgets>

class TabStyle : public QProxyStyle
{
public:
	explicit TabStyle(Qt::Orientation orientation, QStyle * baseStyle = 0);

	QSize sizeFromContents(ContentsType type, const QStyleOption * option,
						   const QSize & size, const QWidget * widget) const;

	void drawControl(ControlElement element, const QStyleOption * option,
					 QPainter * painter, const QWidget * widget) const;

	QTabBar::Shape determineShape(QTabBar::Shape shape) const;

private:
	const Qt::Orientation mOrientation;
};
#endif // TABSTYLE_H
