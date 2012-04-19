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

#include <QMouseEvent>
#include <QVBoxLayout>

#include "plasma-cwp-pixmap-dialog.h"

PixmapDialog::PixmapDialog( QWidget *parent ) :
	Plasma::Dialog( parent )
{
	setResizeHandleCorners( Plasma::Dialog::NoCorner );

	QVBoxLayout *layout = new QVBoxLayout( this );
	label = new QLabel;
	label->setAutoFillBackground ( true );
	label->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );

	layout->addWidget( label );

	setMinimumSize( 50, 50 );

	connect( this, SIGNAL( dialogResized() ), this, SLOT( resized() ) );
	connect( this, SIGNAL( showSignal() ), this, SLOT( show() ) );
}

PixmapDialog::~PixmapDialog()
{
	delete label;
}

void PixmapDialog::getLabelOffset( int &left, int &top, int &right, int &bottom )
{
	left = label->pos().x();
	top = label->pos().y();
	right = size().width() - label->size().width() - label->pos().x();
	bottom = size().height() - label->size().height() - label->pos().y();

	if ( left < 0 || top < 0 || right < 0 || bottom < 0 )
	{
		left = 22;
		top = 22;
		right = 22;
		bottom = 22;
	}
}

void PixmapDialog::setResizable( bool resizable_new )
{
	resizable = resizable_new;

	if ( resizable ) setResizeHandleCorners( Plasma::Dialog::All );
	else setResizeHandleCorners( Plasma::Dialog::NoCorner );
}

void PixmapDialog::setPixmap( const QPixmap &pixmap )
{
	contents_pixmap = pixmap;

	if ( !resizable )
	{
		label->setPixmap( contents_pixmap );
		label->update();
		update();

		int left, top, right, bottom;
		getLabelOffset( left, top, right, bottom );

		int xoffset = size().width();
		int yoffset = size().height();

		resize( QSize( contents_pixmap.width() + left + right, contents_pixmap.height() + top + bottom ) );

		xoffset = xoffset - size().width();
		yoffset = yoffset - size().height();
		move( pos().x() + xoffset, pos().y() + yoffset );

		update();
	}
	else
	{
		resized();
	}
}

void PixmapDialog::setImage( const QImage &image )
{
	setPixmap( QPixmap::fromImage( image ) );
}

void PixmapDialog::resized()
{
	QPixmap pm = contents_pixmap.scaled( label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation );
	label->setPixmap( pm );
}

void PixmapDialog::showEvent( QShowEvent *event )
{
	int xoffset = size().width();
	int yoffset = size().height();

	resized_on_show = false;

	if ( !resizable )
	{
		int left, top, right, bottom;
		getLabelOffset( left, top, right, bottom );

		if ( size() != QSize( contents_pixmap.width() + left + right, contents_pixmap.height() + top + bottom ) )
		{
			resized_on_show = true;
			resize( QSize( contents_pixmap.width() + left + right, contents_pixmap.height() + top + bottom ) );
		}
	}
	else
	{
		resized();
	}

	if ( resized_on_show )
	{
		xoffset = xoffset - size().width();
		yoffset = yoffset - size().height();
		move( pos().x() + xoffset, pos().y() + yoffset );
		emit( showSignal() );
		return;
	}

	Plasma::Dialog::showEvent( event );
}

void PixmapDialog::mousePressEvent( QMouseEvent *event )
{
	if ( event->button() == Qt::LeftButton )
	{
		int left, top, right, bottom;
		getLabelOffset( left, top, right, bottom );

		if ( !isHidden() && QRect( left, top, label->size().width(), label->size().height() ).contains( event->pos(), true ) )
		{
			hide();
			emit( hidden() );
			return;
		}
	}

	Plasma::Dialog::mousePressEvent( event );
}

#include "plasma-cwp-pixmap-dialog.moc"
