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

#include <QTimer>
#include <QGraphicsSceneMouseEvent>

#include "plasma-cwp-graphics-widget.h"

CWP_QGraphicsWidget::CWP_QGraphicsWidget( QGraphicsItem * parent, Qt::WindowFlags wFlags ) :
	QGraphicsWidget( parent, wFlags )
{
	contents_pixmap = QPixmap( QSize( 150, 150 ) );
	contents_pixmap.fill( Qt::transparent );
	setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
}

CWP_QGraphicsWidget::~CWP_QGraphicsWidget()
{
}

void CWP_QGraphicsWidget::paint( QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
	if ( size().toSize() != contents_pixmap.size() )
	{
		// allow fast transformation and ignore aspect ratio, just paint anything so the widget is not empty
		contents_pixmap = contents_pixmap.scaled( size().toSize(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
		emit( pixmapUpdate() );
	}

	p->drawPixmap( 0, 0, contents_pixmap );
	QGraphicsWidget::paint( p, option, widget );
}

QPixmap &CWP_QGraphicsWidget::getPixmap()
{
	return contents_pixmap;
}

void CWP_QGraphicsWidget::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
	emit( mousePressEventReceived( event ) );
}

#include "plasma-cwp-graphics-widget.moc"
