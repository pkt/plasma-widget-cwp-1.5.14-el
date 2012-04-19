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

#include <QColorGroup>
#include <QIcon>
#include <QMovie>
#include <QBuffer>
#include <QLabel>
#include <QMouseEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <KIconLoader>

#include <Plasma/Theme>

#include "plasma-cwp-pixmap-list-dialog.h"

PixmapListDialog::PixmapListDialog( QWidget *parent ) :
	Plasma::Dialog( parent )
{
	current_image = -1;
	movie_buffer = NULL;

	left_icon_hover = false;
	right_icon_hover = false;

	setResizeHandleCorners( Plasma::Dialog::NoCorner );

	QVBoxLayout *layout = new QVBoxLayout( this );

	const int icon_size = 36;
	const int label_size = 40;

	left_icon = new QLabel;
	left_icon->setAutoFillBackground ( true );
	left_icon->setAlignment( Qt::AlignVCenter | Qt::AlignHCenter );
	left_icon_pixmap = KIconLoader::global()->loadIcon( "go-previous", KIconLoader::NoGroup,
		icon_size, KIconLoader::DefaultState, QStringList(), 0L, false );
	left_icon->setPixmap( left_icon_pixmap );
	left_icon->setFixedHeight( label_size );
	left_icon->setFixedWidth( label_size );
	left_icon->setAttribute( Qt::WA_Hover );
	left_icon->installEventFilter( this );

	title = new QLabel;
	QPalette palette = title->palette();
	palette.setColor( QPalette::WindowText, Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
	palette.setColor( QPalette::Text, Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
	palette.setColor( QPalette::BrightText, Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );

	title->setPalette( palette );
	title->setAutoFillBackground ( true );
	title->setAlignment( Qt::AlignVCenter | Qt::AlignHCenter );
	title->setFixedHeight( label_size );

	right_icon = new QLabel;
	right_icon->setAutoFillBackground ( true );
	right_icon->setAlignment( Qt::AlignVCenter | Qt::AlignHCenter );
	right_icon_pixmap = KIconLoader::global()->loadIcon( "go-next", KIconLoader::NoGroup,
		icon_size, KIconLoader::DefaultState, QStringList(), 0L, false );
	right_icon->setPixmap( right_icon_pixmap );
	right_icon->setFixedHeight( label_size );
	right_icon->setFixedWidth( label_size );
	right_icon->setAttribute( Qt::WA_Hover );
	right_icon->installEventFilter( this );

	QHBoxLayout *icon_layout = new QHBoxLayout;
	icon_layout->addWidget( left_icon );
	icon_layout->addWidget( title );
	icon_layout->addWidget( right_icon );

	layout->addLayout( icon_layout );

	background_label = new QLabel;
	background_label->setAutoFillBackground ( true );
	background_label->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
	background_label->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

	label = new QLabel( background_label );
	label->setAutoFillBackground ( true );
	label->setAlignment( Qt::AlignHCenter | Qt::AlignVCenter );
	label->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
	label->resize( background_label->size() );

	label->installEventFilter( this );

	layout->addWidget( background_label );

	setMinimumSize( 50, 50 );

	QPainterPath path;
	path.addRoundRect( QRectF( 0, 0, label_size - 2, label_size - 2 ), 15 );
	QLinearGradient gradient( 0.5, 0, 0.5, 1 );
	gradient.setCoordinateMode( QGradient::ObjectBoundingMode );
	if ( qGray( Plasma::Theme::BackgroundColor ) < 140 )
	{
		gradient.setColorAt( 0, QColor( 240, 240, 240, 255 ) );
		gradient.setColorAt( 1, QColor( 140, 140, 140, 255 ) );
	}
	else
	{
		gradient.setColorAt( 0, QColor( 140, 140, 140, 255 ) );
		gradient.setColorAt( 1, QColor( 40, 40, 40, 255 ) );
	}

	left_icon_pixmap_lighter = QPixmap( label_size, label_size );
	left_icon_pixmap_lighter.fill( Qt::transparent );
	QPainter painter_left( &left_icon_pixmap_lighter );
	painter_left.setBrush( gradient );
	painter_left.translate( 1, 1 );
	painter_left.drawPath( path );
	painter_left.translate( -1, -1 );
	painter_left.drawPixmap( QRect( (int)(0.5*(label_size-icon_size)), (int)(0.5*(label_size-icon_size)),
		left_icon_pixmap.width(), left_icon_pixmap.height() ), left_icon_pixmap );

	right_icon_pixmap_lighter = QPixmap( label_size, label_size );
	right_icon_pixmap_lighter.fill( Qt::transparent );
	QPainter painter_right( &right_icon_pixmap_lighter );
	painter_right.setBrush( gradient );
	painter_left.translate( 0.5, 0.5 );
	painter_right.drawPath( path );
	painter_left.translate( -0.5, -0.5 );
	painter_right.drawPixmap( QRect( (int)(0.5*(label_size-icon_size)), (int)(0.5*(label_size-icon_size)),
		right_icon_pixmap.width(), right_icon_pixmap.height() ), right_icon_pixmap );

	connect( this, SIGNAL( dialogResized() ), this, SLOT( resized() ) );
}

PixmapListDialog::~PixmapListDialog()
{
	if ( label->movie() ) delete label->movie();
	if ( movie_buffer ) delete movie_buffer;
	delete label;
	delete background_label;
	delete left_icon;
	delete right_icon;
}

void PixmapListDialog::getLabelOffset( int &left, int &top, int &right, int &bottom )
{
	left = background_label->pos().x();
	top = background_label->pos().y();
	right = size().width() - background_label->size().width() - background_label->pos().x();
	bottom = size().height() - background_label->size().height() - background_label->pos().y();

	if ( left < 0 || top < 0 || right < 0 || bottom < 0 )
	{
		left = 22;
		top = 22;
		right = 22;
		bottom = 22;
	}
}

void PixmapListDialog::setResizable( bool resizable_new )
{
	resizable = resizable_new;

	if ( resizable ) setResizeHandleCorners( Plasma::Dialog::All );
	else setResizeHandleCorners( Plasma::Dialog::NoCorner );
}

void PixmapListDialog::setImageList( const QList<QByteArray> &list )
{
	contents_image_list = list;

	if ( contents_image_list.size() <= 0 ) return;

	if ( current_image < 0 || current_image > contents_image_list.size() )
	{
		current_image = 0;
		left_icon->setPixmap( QPixmap() );
		left_icon_hover = false;
		if ( contents_image_list.size() > 1 )
		{
			if ( right_icon_hover ) right_icon->setPixmap( right_icon_pixmap_lighter );
			else right_icon->setPixmap( right_icon_pixmap );
		}
	}

	if ( current_image == 0 )
	{
		left_icon->setPixmap( QPixmap() );
		left_icon_hover = false;
	}
	else
	{
		if ( left_icon_hover ) left_icon->setPixmap( left_icon_pixmap_lighter );
		else left_icon->setPixmap( left_icon_pixmap );
	}

	if ( current_image < contents_image_list.size() - 1 )
	{
		if ( right_icon_hover ) right_icon->setPixmap( right_icon_pixmap_lighter );
		else right_icon->setPixmap( right_icon_pixmap );
	}
	else
	{
		right_icon->setPixmap( QPixmap() );
		right_icon_hover = false;
	}

	if ( !resizable )
	{
		int left, top, right, bottom;
		getLabelOffset( left, top, right, bottom );

		QImage tmp_image;
		tmp_image.loadFromData( contents_image_list.at( current_image ) );

		resize( QSize( tmp_image.width() + left + right, tmp_image.height() + top + bottom ) );
		setMovie( contents_image_list.at( current_image ) );
	}
	else
	{
		resized();
	}
}

void PixmapListDialog::setNameList( const QStringList &list )
{
	contents_name_list = list;
	if ( contents_name_list.size() == 1 )
	{
		if ( contents_name_list.at( 0 ) == "" )
		{
			left_icon->hide();
			title->hide();
			right_icon->hide();
		}
		else
		{
			left_icon->show();
			title->show();
			right_icon->show();
		}
	}
	else
	{
		left_icon->show();
		title->show();
		right_icon->show();
	}
}

void PixmapListDialog::setCurrentImage( const int current )
{
	current_image = current;
	if ( contents_image_list.size() <= 0 )
	{
		current_image = -1;
	}
	else
	{
		if ( current_image < 0 ) current_image = 0;
		else if ( current_image > contents_image_list.size() - 1 ) current_image = contents_image_list.size() - 1;
	}
	resized();
}

void PixmapListDialog::setFont( const QFont &font )
{
	QFont new_font = font;
	new_font.setPixelSize( 14 );
	new_font.setWeight( QFont::Bold );
	title->setFont( new_font );
}

void PixmapListDialog::setMovie( const QByteArray& image )
{
	if ( label->movie() ) delete label->movie();
	if ( movie_buffer ) delete movie_buffer;

	movie_buffer = new QBuffer();
	movie_buffer->buffer() = image;
	movie_buffer->open( QIODevice::ReadOnly );

	QMovie *pm = new QMovie( movie_buffer );

	QBuffer *movie_buffer_tmp = new QBuffer();
	movie_buffer_tmp->buffer() = image;
	movie_buffer_tmp->open( QIODevice::ReadOnly );
	QMovie *pm_tmp = new QMovie( movie_buffer_tmp );
	pm_tmp->jumpToFrame ( 0 );
	QImage background_image = pm_tmp->currentImage();
	QSize scaled_size = pm_tmp->currentImage().size();
	pm_tmp->stop();
	delete pm_tmp;
	delete movie_buffer_tmp;

	scaled_size.scale( background_label->size(), Qt::KeepAspectRatio );
	pm->setScaledSize( scaled_size );
	background_label->setPixmap( QPixmap::fromImage( background_image.scaled( scaled_size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation ) ) );
	pm->setCacheMode( QMovie::CacheAll );
	label->setMovie( pm );
	label->movie()->start();
}

void PixmapListDialog::resized()
{
	if ( current_image >= 0 && contents_image_list.size() > 0 && current_image <= contents_image_list.size() - 1 )
	{
		if ( current_image == 0 )
		{
			left_icon->setPixmap( QPixmap() );
			left_icon_hover = false;
		}
		else
		{
			if ( left_icon_hover ) left_icon->setPixmap( left_icon_pixmap_lighter );
			else left_icon->setPixmap( left_icon_pixmap );
		}

		if ( current_image < contents_image_list.size() - 1 )
		{
			if ( right_icon_hover ) right_icon->setPixmap( right_icon_pixmap_lighter );
			else right_icon->setPixmap( right_icon_pixmap );
		}
		else
		{
			right_icon->setPixmap( QPixmap() );
			right_icon_hover = false;
		}

		setMovie( contents_image_list.at( current_image ) );

		if ( current_image >= 0 && contents_name_list.size() > 0 && current_image <= contents_name_list.size() - 1 )
		{
			title->setText( contents_name_list.at( current_image ) );
		}
		else
		{
			title->setText( "" );
		}
	}

	left_icon->update();
	right_icon->update();
	title->update();
	background_label->update();
	label->resize( background_label->size() );
	label->update();
}

void PixmapListDialog::showEvent( QShowEvent *event )
{
	if ( resizable )
	{
		resized();
	}

	Plasma::Dialog::showEvent( event );
}

bool PixmapListDialog::eventFilter( QObject *obj, QEvent *event )
{
	if ( obj == label )
	{
		if ( event->type() == QEvent::MouseButtonPress && ((QMouseEvent*)event)->button() == Qt::LeftButton && !isHidden() )
		{
			hide();
			emit( hidden() );
			left_icon_hover = false;
			right_icon_hover = false;
			return true;
		}

		if ( event->type() == QEvent::Wheel && !isHidden() )
		{
			int shift = ((QWheelEvent*)event)->delta() / 120;
			label->movie()->stop();
			label->movie()->jumpToFrame( label->movie()->currentFrameNumber() + shift );
			return true;
		}

		if ( event->type() == QEvent::MouseButtonPress && ((QMouseEvent*)event)->button() == Qt::MidButton && !isHidden() )
		{
			switch ( label->movie()->state() )
			{
				case QMovie::NotRunning:
					label->movie()->start();
					break;

				case QMovie::Paused:
					label->movie()->setPaused( false );
					break;

				case QMovie::Running:
					label->movie()->setPaused( true );
					break;

				default:
				break;
			}

			return true;
		}
	}

	if ( obj == left_icon )
	{
		if ( ( event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick ) && ((QMouseEvent*)event)->button() == Qt::LeftButton && !isHidden() && !left_icon->isHidden() && contents_image_list.size() > 0 ) // back
		{
			current_image--;
			if ( current_image < 0 ) current_image = 0;
			emit( currentImageChanged( current_image ) );
			resized();
			return true;
		}

		if ( event->type() == QEvent::HoverEnter && !left_icon->pixmap()->isNull() )
		{
			left_icon->setPixmap( left_icon_pixmap_lighter );
			left_icon_hover = true;
			return false;
		}

		if ( event->type() == QEvent::HoverLeave && !left_icon->pixmap()->isNull() )
		{
			left_icon->setPixmap( left_icon_pixmap );
			left_icon_hover = false;
			return false;
		}
	}

	if ( obj == right_icon )
	{
		if ( ( event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonDblClick ) && !isHidden() && !right_icon->isHidden() && contents_image_list.size() > 0 ) // forward
		{
			current_image++;
			if ( current_image > contents_image_list.size() - 1 ) current_image = contents_image_list.size() - 1;
			emit( currentImageChanged( current_image ) );
			resized();
			return true;
		}

		if ( event->type() == QEvent::HoverEnter && !right_icon->pixmap()->isNull() )
		{
			right_icon->setPixmap( right_icon_pixmap_lighter );
			right_icon_hover = true;
			return false;
		}

		if ( event->type() == QEvent::HoverLeave && !right_icon->pixmap()->isNull() )
		{
			right_icon->setPixmap( right_icon_pixmap );
			right_icon_hover = false;
			return false;
		}
	}

	return false;
}

#include "plasma-cwp-pixmap-list-dialog.moc"
