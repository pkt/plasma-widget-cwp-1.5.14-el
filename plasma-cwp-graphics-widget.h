/***************************************************************************
*   Copyright (C) 2009 by Georg Hennig <georg.hennig@web.de>              *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
***************************************************************************/

#ifndef Plasma_CWP_Graphics_Widget_HEADER
#define Plasma_CWP_Graphics_Widget_HEADER

#include <QGraphicsWidget>
#include <QPainter>

class CWP_QGraphicsWidget : public QGraphicsWidget
{
	Q_OBJECT

	public:
		CWP_QGraphicsWidget( QGraphicsItem * parent = 0, Qt::WindowFlags wFlags = 0 );
		~CWP_QGraphicsWidget();
		virtual void paint( QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget );
		QPixmap &getPixmap();

		void setLocationImageRect( const QRect &rect );
		void setCurrentWeatherRect( const QRect &rect1, const QRect &rect2, const QRect &rect3, const QRect &rect4, const QRect &rect5 );

	protected:
		void mousePressEvent( QGraphicsSceneMouseEvent *event );

	signals:
		void mousePressEventReceived( QGraphicsSceneMouseEvent *event );
		void pixmapUpdate();

	private:
		QPixmap contents_pixmap;
};

#endif
