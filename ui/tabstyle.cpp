#include "ui/tabstyle.h"

TabStyle::TabStyle(Qt::Orientation orientation, QStyle *baseStyle)
	: QProxyStyle(baseStyle), mOrientation(orientation)
{ }

QSize TabStyle::sizeFromContents(ContentsType type,const QStyleOption *option, const QSize &size, const QWidget *widget) const
{
	QSize s = QProxyStyle::sizeFromContents(type, option, size, widget);
	if (type != QStyle::CT_TabBarTab)
		return s;

	if (const QStyleOptionTab * tab = qstyleoption_cast<const QStyleOptionTab *>(option))
	{
		switch (tab->shape)
		{
			case QTabBar::RoundedWest:
			case QTabBar::RoundedEast:
			case QTabBar::TriangularWest:
			case QTabBar::TriangularEast:
				if (mOrientation == Qt::Horizontal)
					s.transpose();
				break;
			case QTabBar::RoundedNorth:
			case QTabBar::RoundedSouth:
			case QTabBar::TriangularNorth:
			case QTabBar::TriangularSouth:
				if (mOrientation == Qt::Vertical)
					s.transpose();
			default:
				break;
		}
	}
	return s;
}


void TabStyle::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const {
	if (element == CE_TabBarTabLabel)
	{
		if (const QStyleOptionTab * tab = qstyleoption_cast<const QStyleOptionTab *>(option))
		{
			QStyleOptionTab opt(*tab);
			opt.shape = determineShape(tab->shape);
			return QProxyStyle::drawControl(element, &opt, painter, widget);
		}
	}
	QProxyStyle::drawControl(element, option, painter, widget);
}

QTabBar::Shape TabStyle::determineShape(QTabBar::Shape shape) const {
	{
		if (mOrientation == Qt::Horizontal)
		{
			switch(shape)
			{
				case QTabBar::RoundedWest:      shape = QTabBar::RoundedNorth;      break;
				case QTabBar::RoundedEast:      shape = QTabBar::RoundedSouth;      break;
				case QTabBar::TriangularWest:   shape = QTabBar::TriangularNorth;   break;
				case QTabBar::TriangularEast:   shape = QTabBar::TriangularSouth;   break;
				default:                                                            break;
			}
		}
		else if (mOrientation == Qt::Vertical)
		{
			switch(shape)
			{
				case QTabBar::RoundedNorth:     shape = QTabBar::RoundedWest;       break;
				case QTabBar::RoundedSouth:     shape = QTabBar::RoundedWest;       break;
				case QTabBar::RoundedEast:      shape = QTabBar::RoundedWest;       break;
				case QTabBar::TriangularNorth:  shape = QTabBar::TriangularWest;    break;
				case QTabBar::TriangularSouth:  shape = QTabBar::TriangularWest;    break;
				case QTabBar::TriangularEast:   shape = QTabBar::TriangularWest;    break;
				default:                                                            break;
			}
		}
		return shape;
	}
}
