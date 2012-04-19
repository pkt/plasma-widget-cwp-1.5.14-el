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

#include <QPainter>

#include <Plasma/Applet>
#include <Plasma/PaintUtils>
#include <Plasma/Svg>

#include <KIconLoader>

#include "plasma-cwp-paint-helper.h"

PaintHelper::PaintHelper( Plasma::PopupApplet *par ) :
	parent( par )
{
}

PaintHelper::~PaintHelper()
{
}

void PaintHelper::setValues(
	const QString &humidity_tmp,
	const QString &wind_tmp,
	const bool &invertWindRose_tmp,
	const double &windRoseScale_tmp,
	const QString &rain_tmp,
	const QString &dewPoint_tmp,
	const QString &visibility_tmp,
	const QString &pressure_tmp,
	const QString &uvIndex_tmp,
	const QString &tempType_tmp,
	const int &icons_tmp,
	const KUrl &iconsCustom_tmp,
	const int &background_tmp,
	const QColor &backgroundColor_tmp,
	const int &backgroundTransparency_tmp,
	const double &scaleIcons_tmp,
	const QString &font_tmp,
	const QColor &fontColor_tmp,
	const bool &fontShadow_tmp,
	const double &scaleFont_tmp,
	const int &forecastSeparator_tmp,
	const int &layoutDirection_tmp,
	const QByteArray &data_custom_image_tmp,
	const QString &data_current_wind_tmp,
	const QString &data_current_humidity_tmp,
	const QString &data_current_rain_tmp,
	const QString &data_current_dew_point_tmp,
	const QString &data_current_visibility_tmp,
	const QString &data_current_pressure_tmp,
	const QString &data_current_uv_index_tmp
	)
{
	humidity = humidity_tmp;
	wind = wind_tmp;
	invertWindRose = invertWindRose_tmp;
	windRoseScale = windRoseScale_tmp;
	rain = rain_tmp;
	dewPoint = dewPoint_tmp;
	visibility = visibility_tmp;
	pressure = pressure_tmp;
	uvIndex = uvIndex_tmp;
	tempType = tempType_tmp;
	icons = icons_tmp;
	iconsCustom = iconsCustom_tmp;
	background = background_tmp;
	backgroundColor = backgroundColor_tmp;
	backgroundTransparency = backgroundTransparency_tmp;
	scaleIcons = scaleIcons_tmp;
	font = font_tmp;
	fontColor = fontColor_tmp;
	fontShadow = fontShadow_tmp;
	scaleFont = scaleFont_tmp;
	forecastSeparator = forecastSeparator_tmp;
	layoutDirection = layoutDirection_tmp;
	data_custom_image.loadFromData( data_custom_image_tmp );
	data_current_wind = data_current_wind_tmp;
	data_current_humidity = data_current_humidity_tmp;
	data_current_rain = data_current_rain_tmp;
	data_current_dew_point = data_current_dew_point_tmp;
	data_current_visibility = data_current_visibility_tmp;
	data_current_pressure = data_current_pressure_tmp;
	data_current_uv_index = data_current_uv_index_tmp;
}

void PaintHelper::getRectCurrentHoriz( const int x_0, const int y_0, const int width, const int height, const double x_divisor,
									  const double y_divisor, const double x_start, const double y_start, QRect &complete_rect,
									  QRect &temp_rect, QRect &icon_rect, QRect &icon_text_rect, QRect &wind_rect, QRect &sun_rect )
{
	int height_eff = (int)( (double)(height) / y_divisor );
	int width_eff = (int)( (double)(width) / x_divisor );

	int x_offset = (int)( x_start * width_eff );
	int y_offset = (int)( y_start * height_eff );
	int width_offset = 0;
	int height_offset = 0;

	// The rest missing to 100% is space between current and forcast information
	double wind_percentage = 0.82;

	double icon_text_percentage = 0.16;

	complete_rect = QRect( 0, 0, 0, 0/*x_0 + x_offset, y_0, 3 * width_eff, height_eff*/ );
	if ( forecastSeparator == 1 ) /* cwp style */
	{
		x_offset += 3;
		width_offset -= 6;
		y_offset += 3;
		height_offset -= 3;
	}
	else if ( forecastSeparator == 2 ) /* plasma style */
	{
		qreal left, top, right, bottom;
		parent->getContentsMargins( &left, &top, &right, &bottom );

		if ( background == 2 || background == 3 || parent->size().height() < 150. ) /* none or custom or panel mode: Plasma::Applet::NoBackground */
		{
			parent->setBackgroundHints( Plasma::Applet::TranslucentBackground );
			parent->getContentsMargins( &left, &top, &right, &bottom );
			parent->setBackgroundHints( Plasma::Applet::NoBackground );
		}

		x_offset += (int)( left );
		width_offset -= (int)( left + right );
		y_offset += (int)( top );
		height_offset -= (int)( top + bottom );
	}

	temp_rect = QRect( x_0 + x_offset,
					   y_0 + y_offset,
					   width_eff + width_offset,
					   (int)((1.-icon_text_percentage)*(height_eff+height_offset)) );
	icon_rect = QRect( x_0 + x_offset + width_eff,
					   y_0 + y_offset,
					   width_eff + width_offset,
					   (int)((1.-icon_text_percentage)*(height_eff+height_offset)) );
	icon_text_rect = QRect( x_0 + x_offset + width_eff,
							y_0 + y_offset + (int)(0.5*(1.-icon_text_percentage)*(height_eff+height_offset)),
							width_eff + width_offset,
							(int)((0.5*(1.-icon_text_percentage)+icon_text_percentage)*(height_eff+height_offset)) );
	wind_rect = QRect( x_0 + x_offset + 2 * width_eff,
					   y_0 + y_offset,
					   width_eff + width_offset,
					   (int)(wind_percentage*(height_eff+height_offset) ) );
	sun_rect	= QRect( x_0 + x_offset + 2 * width_eff,
						 y_0 + y_offset + height_eff + height_offset - (int)((1.-wind_percentage)*(height_eff+height_offset)),
						 width_eff + width_offset,
						 (int)((1.-wind_percentage)*(height_eff+height_offset)) );
}

void PaintHelper::getRectCurrentVert( const int x_0, const int y_0, const int width, const int height,
									 const double x_divisor, const double y_divisor, const double x_start,
									 const double y_start, QRect &complete_rect, QRect &temp_rect, QRect &icon_rect,
									 QRect &icon_text_rect, QRect &wind_rect, QRect &sun_rect )
{
	int height_eff = (int)( (double)(height) / y_divisor );
	int width_eff = (int)( (double)(width) / x_divisor );

	int x_offset = (int)( x_start * width_eff );
	int y_offset = (int)( y_start * height_eff );
	int width_offset = 0;
	int height_offset = 0;

	// The rest missing to 100% is space between current and forcast information
	double space_percentage = 0.1;
	if ( forecastSeparator == 2 ) space_percentage = 0; // plasma style
	double icon_text_percentage = 0.16;
	double wind_percentage = 0.82;

	complete_rect = QRect( 0, 0, 0, 0/*x_0, y_0, width, height_eff*/ );
	if ( forecastSeparator == 1 ) /* cwp style */
	{
		x_offset += 3;
		width_offset -= 6;
		y_offset += 3;
		height_offset -= 3;
	}
	else if ( forecastSeparator == 2 ) /* plasma style */
	{
		qreal left, top, right, bottom;
		parent->getContentsMargins( &left, &top, &right, &bottom );

		if ( background == 2 || background == 3 || parent->size().height() < 150. ) /* none or custom or panel mode: Plasma::Applet::NoBackground */
		{
			parent->setBackgroundHints( Plasma::Applet::TranslucentBackground );
			parent->getContentsMargins( &left, &top, &right, &bottom );
			parent->setBackgroundHints( Plasma::Applet::NoBackground );
		}

		x_offset += (int)( left );
		width_offset -= (int)( left + right );
		y_offset += (int)( top );
		height_offset -= (int)( top + bottom );
	}

	temp_rect = QRect( x_0 + x_offset,
					   y_0 + y_offset,
					   width_eff + width_offset,
					   (int)((1.-space_percentage)*(height_eff+height_offset)) );
	icon_rect = QRect( x_0 + x_offset + width_eff,
					   y_0 + y_offset,
					   width_eff + width_offset,
					   (int)((1.-icon_text_percentage)*(1.-space_percentage)*(height_eff+height_offset)) );
	icon_text_rect = QRect( x_0 + x_offset + width_eff,
							y_0 + y_offset + (int)(0.5*(1.-icon_text_percentage)*(1.-space_percentage)*(height_eff+height_offset)),
							width_eff + width_offset,
							(int)((0.5*(1.-icon_text_percentage)+icon_text_percentage)*(1.-space_percentage)*(height_eff+height_offset)) );
	wind_rect = QRect( x_0 + x_offset,
					   y_0 + y_offset + (int)((1.+space_percentage)*height_eff),
					   width_eff + width_offset,
					   (int)(wind_percentage*(1.-space_percentage)*(height_eff+height_offset)) );
	sun_rect	= QRect( x_0 + x_offset,
						 y_0 + y_offset + height_eff + (height_eff+height_offset) - (int)((1.-wind_percentage)*(1.-space_percentage)*(height_eff+height_offset)),
						 width_eff + width_offset,
						 (int)((1.-wind_percentage)*(1.-space_percentage)*(height_eff+height_offset)) );
}

void PaintHelper::getRectCurrentLoc( const int x_0, const int y_0, const int width, const int height, const double x_divisor,
									const double y_divisor, const double x_start, const double y_start, QRect &complete_rect,
									QRect &temp_rect, QRect &wind_rect, QRect &sun_rect )
{
	Q_UNUSED( x_start );
	Q_UNUSED( y_start );

	int x_offset = 0;
	int y_offset = 0;
	int width_offset = 0;
	int height_offset = 0;
	int height_eff = (int)( (double)(height) / y_divisor );
	int width_eff = (int)( (double)(width) / x_divisor );

	if ( forecastSeparator == 1 ) /* cwp style */
	{
		x_offset += 3;
		width_offset -= 6;
		y_offset += 3;
		height_offset -= 3;
	}
	else if ( forecastSeparator == 2 ) /* plasma style */
	{
		qreal left, top, right, bottom;
		parent->getContentsMargins( &left, &top, &right, &bottom );

		if ( background == 2 || background == 3 || parent->size().height() < 150. ) /* none or custom or panel mode: Plasma::Applet::NoBackground */
		{
			parent->setBackgroundHints( Plasma::Applet::TranslucentBackground );
			parent->getContentsMargins( &left, &top, &right, &bottom );
			parent->setBackgroundHints( Plasma::Applet::NoBackground );
		}

		x_offset += (int)( left );
		width_offset -= (int)( left + right );
		y_offset += (int)( top );
		height_offset -= (int)( top + bottom );
	}

	// The rest missing to 100% is sunrise/sunset space
	double wind_percentage = 0.82;

	complete_rect = QRect( 0, 0, 0, 0 );
	temp_rect = QRect( x_0 + x_offset + (int)( x_start * width_eff + 1.5 * width_eff ),
					   y_0 + y_offset + (int)( y_start*height_eff ),
					   width_eff + width_offset,
					   (int)((height_eff+height_offset)) );
	wind_rect = QRect( x_0 + x_offset + (int)( x_start*width_eff + 2.5 * width_eff ),
					   y_0 + y_offset + (int)( y_start*height_eff ),
					   width_eff + width_offset,
					   (int)(wind_percentage*(height_eff+height_offset) ) );
	sun_rect	= QRect( x_0 + x_offset + (int)( x_start*width_eff + 2.5 * width_eff ),
						 y_0 + y_offset + (int)( y_start*height_eff ) + height_eff + height_offset - (int)((1.-wind_percentage)*(height_eff+height_offset)),
						 width_eff + width_offset,
						 (int)((1.-wind_percentage)*(height_eff+height_offset)) );
}

void PaintHelper::getRectLocationImage( const int x_0, const int y_0, const int width, const int height, const double x_divisor,
									   const double y_divisor, const double x_start, const double y_start, QRect &custom_image_rect,
									   QRect &icon_rect, QRect &icon_text_rect, QRect &title_rect, QRect &clock_rect )
{
	Q_UNUSED( y_divisor );
	Q_UNUSED( x_start );
	Q_UNUSED( y_start );

	// missing to 1.0 is used as space between title and image
	double clock_percentage = 0.05;
	double icon_percentage = 0.25;
	double icon_text_percentage = 0.07;
	double title_percentage = 0.15;
	double custom_image_percentage = 0.48;

	int height_eff = height;
	int width_eff = (int)( 1.5 * (double)(width) / x_divisor );

	clock_rect = QRect( x_0,
						y_0,
						width_eff,
						(int)(clock_percentage*height_eff) );
	icon_rect = QRect( x_0,
					   y_0 + (int)(clock_percentage*height_eff),
					   width_eff,
					   (int)(icon_percentage*height_eff) );
	icon_text_rect = QRect( x_0,
							y_0 + (int)((clock_percentage + 0.5*icon_percentage)*height_eff),
							width_eff,
							(int)((0.5*icon_percentage+icon_text_percentage)*height_eff) );
	title_rect = QRect( x_0,
						y_0 + (int)(clock_percentage*height_eff + icon_percentage*height_eff + icon_text_percentage*height_eff),
						width_eff,
						(int)(title_percentage*height_eff) );
	custom_image_rect = QRect( x_0,
								 y_0 + (int)((1.-custom_image_percentage)*height_eff),
								 width_eff,
								 (int)(custom_image_percentage*height_eff) );
}

void PaintHelper::getRectForecast( const int x_0, const int y_0, const int width, const int height, const double x_divisor,
								  double y_divisor, const double x_start, const double y_start, QRect &complete_rect, QRect &name_rect,
								  QRect &temp_rect, QRect &icon_rect, QRect &icon_text_rect )
{
	double title_percentage = 0.20;
	double icon_text_percentage = 0.16;
	double temperature_percentage = 0.20;
	double space_percentage = 0.0;

	int x_offset = 0;
	int y_offset = 0;
	int width_offset = 0;
	int height_offset = 0;
	int height_eff = (int)( (double)(height) / y_divisor );
	int width_eff = (int)( (double)(width) / x_divisor );

	complete_rect = QRect( (int)( x_0 + x_start * width_eff ), (int)( y_0 + y_start * height_eff ), width_eff, height_eff );
	if ( forecastSeparator == 1 ) /* cwp style */
	{
		x_offset += 3;
		width_offset -= 6;
		y_offset += 3;
		height_offset -= 3;
	}
	else if ( forecastSeparator == 2 ) /* plasma style */
	{
		qreal left, top, right, bottom;
		parent->getContentsMargins( &left, &top, &right, &bottom );

		if ( background == 2 || background == 3 || parent->size().height() < 150. ) /* none or custom or panel mode: Plasma::Applet::NoBackground */
		{
			parent->setBackgroundHints( Plasma::Applet::TranslucentBackground );
			parent->getContentsMargins( &left, &top, &right, &bottom );
			parent->setBackgroundHints( Plasma::Applet::NoBackground );
		}

		x_offset += (int)( left );
		width_offset -= (int)( left + right );
		y_offset += (int)( top );
		height_offset -= (int)( top + bottom );
	}

	name_rect = QRect( (int)(x_0 + x_offset + x_start * width_eff),
					   (int)(y_0 + y_offset + y_start * height_eff),
					   width_eff + width_offset,
					   (int)((height_eff+height_offset) * title_percentage) );
	temp_rect = QRect( (int)(x_0 + x_offset + x_start * width_eff),
					   (int)(y_0 + y_offset + y_start * height_eff + (int)((height_eff+height_offset) * title_percentage)),
					   width_eff +height_offset,
					   (int)((height_eff+height_offset) * temperature_percentage) );
	icon_rect = QRect( (int)(x_0 + x_offset + x_start * width_eff),
					   (int)(y_0 + y_offset + y_start * height_eff + (int)((height_eff+height_offset) * (title_percentage+temperature_percentage))),
					   (int)(width_eff + width_offset),
					   (int)((height_eff+height_offset) * (1.-title_percentage-icon_text_percentage-temperature_percentage-space_percentage)) );
	icon_text_rect = QRect( (int)(x_0 + x_offset + x_start * width_eff),
							(int)(y_0 + y_offset + y_start * height_eff + (int)((height_eff+height_offset) * (1.-icon_text_percentage-space_percentage-0.5*(title_percentage+temperature_percentage)))),
							(int)(width_eff + width_offset),
							(int)((height_eff+height_offset) * (icon_text_percentage-space_percentage+0.5*(title_percentage+temperature_percentage))) );
}

void PaintHelper::drawBackground( QPainter *p, QRect &contents_rect )
{
	if ( contents_rect.width() == 0 || contents_rect.height() == 0 ) return;

	QPixmap pixmap( contents_rect.size() );
	pixmap.fill( Qt::transparent );

	QPainter painter( &pixmap );

	painter.setPen( p->pen() );
	painter.setFont( p->font() );

	QRect contents_rect_gradient( QPoint( 0, 0 ), contents_rect.size() );

	QPainterPath path;
	path.addRoundRect( contents_rect_gradient.adjusted( 0, 0, -1, -1 ), 5 );

	QColor color = backgroundColor/*QColor(127,127,127)*/;

	QLinearGradient gradient( 0, 0, 0, 1 );
	gradient.setCoordinateMode( QGradient::ObjectBoundingMode );
	gradient.setColorAt( 0, color );
	gradient.setColorAt( 1, color );

	painter.translate( 0.5, 0.5 );
	if ( parent->size().height() > 150. ) /* don't draw border in panel mode */
	{
		if ( qGray( color.rgb() ) < 192 )
		{
			painter.setPen( QPen( Qt::white, 1 ) );
		}
		else
		{
			painter.setPen( QPen( Qt::black, 1 ) );
		}
	}
	painter.setBrush( gradient );
	painter.drawPath( path );
	painter.translate( -0.5, -0.5 );


	gradient.setCoordinateMode( QGradient::ObjectBoundingMode );
	gradient.setColorAt( 0, QColor( 0, 0, 0, backgroundTransparency ) );
	gradient.setColorAt( 1, QColor( 0, 0, 0, backgroundTransparency ) );
	painter.setCompositionMode( QPainter::CompositionMode_DestinationIn );
	painter.fillRect( contents_rect, gradient );
	painter.setCompositionMode( QPainter::CompositionMode_SourceOver );

	p->drawPixmap( contents_rect, pixmap );

	int border = (int)(0.015*contents_rect.width() > 0.015*contents_rect.height() ? 0.015*contents_rect.width() : 0.015*contents_rect.height());
	contents_rect = QRect( contents_rect.x() + border, contents_rect.y() + border, contents_rect.width() - 2 * border, contents_rect.height() - 2 * border );
}

void PaintHelper::drawWind( QPainter *p, const QRect &rectangle, const QString &code, const QString &speed, const QString &title,
						   const QString &rose_text, const QFont &speed_font, const QFont &nsew_font )
{
	Q_UNUSED( title );

	double add_rotation = 0.0;
	if ( invertWindRose ) add_rotation = 180.0;

	QTransform transform;
	int arrow_var_unknown = 0; // 0=arrow, 1=var, 2=unknown
	if ( code == "N" )				transform.rotate( -270.0 + add_rotation, Qt::ZAxis );
	else if ( code == "NNE" ) transform.rotate( -247.5 + add_rotation, Qt::ZAxis );
	else if ( code == "NE" )	transform.rotate( -225.0 + add_rotation, Qt::ZAxis );
	else if ( code == "ENE" ) transform.rotate( -202.5 + add_rotation, Qt::ZAxis );
	else if ( code == "E" )	 transform.rotate( -180.0 + add_rotation, Qt::ZAxis );
	else if ( code == "ESE" ) transform.rotate( -157.5 + add_rotation, Qt::ZAxis );
	else if ( code == "SE" )	transform.rotate( -135.0 + add_rotation, Qt::ZAxis );
	else if ( code == "SSE" ) transform.rotate( -112.5 + add_rotation, Qt::ZAxis );
	else if ( code == "S" )	 transform.rotate( -90.0 + add_rotation, Qt::ZAxis );
	else if ( code == "SSW" ) transform.rotate( -67.5 + add_rotation, Qt::ZAxis );
	else if ( code == "SW" )	transform.rotate( -45.0 + add_rotation, Qt::ZAxis );
	else if ( code == "WSW" ) transform.rotate( -22.5 + add_rotation, Qt::ZAxis );
	else if ( code == "W" )	 transform.rotate( -0.0 + add_rotation, Qt::ZAxis );
	else if ( code == "WNW" ) transform.rotate( -337.5 + add_rotation, Qt::ZAxis );
	else if ( code == "NW" )	transform.rotate( -315.0 + add_rotation, Qt::ZAxis );
	else if ( code == "NNW" ) transform.rotate( -292.5 + add_rotation, Qt::ZAxis );
	else if ( code == "Var" ) arrow_var_unknown = 1;
	else											arrow_var_unknown = 2;

	QString svg_file = QString();

	if ( arrow_var_unknown == 0 ) svg_file = "widgets/wind_arrow";
	else if ( arrow_var_unknown == 1 ) svg_file = "widgets/wind_var";
	else svg_file = "widgets/wind_unknown";

	Plasma::Svg* svg;
	svg = new Plasma::Svg( this );
	svg->setImagePath( svg_file );

	QStringList NSEW = rose_text.split( ";", QString::KeepEmptyParts, Qt::CaseSensitive );

	double wind_speed_percentage = 0.23;
	double wind_rose_percentage = 1.0;

	int rose_size = rectangle.width() > (int)((1.-wind_speed_percentage)*rectangle.height()) ? (int)((1.-wind_speed_percentage)*rectangle.height()) : rectangle.width();

	QFont nsew_new_font = nsew_font;
	int max_size = 0;
	int size_tmp;
	if ( NSEW.size() >= 4 )
	{
		nsew_new_font = getFittingFontSize( p, QRect( rectangle.x(), rectangle.y(), (int)( 0.22 * rose_size ), (int)( 0.22 * rose_size ) ),
			Qt::AlignTop | Qt::AlignHCenter, NSEW.at(0), nsew_font, true, true, 0.3, 0, 0 );
		p->setFont( nsew_new_font );

		size_tmp = p->boundingRect( rectangle, Qt::AlignTop | Qt::AlignHCenter, NSEW.at(0) ).height();
		if ( size_tmp > max_size ) max_size = size_tmp;

		size_tmp = p->boundingRect( rectangle, Qt::AlignBottom | Qt::AlignHCenter, NSEW.at(1) ).height();
		if ( size_tmp > max_size ) max_size = size_tmp;

		size_tmp = p->boundingRect( rectangle, Qt::AlignVCenter | Qt::AlignRight, NSEW.at(2) ).width();
		if ( size_tmp > max_size ) max_size = size_tmp;

		size_tmp = p->boundingRect( rectangle, Qt::AlignVCenter | Qt::AlignLeft, NSEW.at(3) ).width();
		if ( size_tmp > max_size ) max_size = size_tmp;

		if ( max_size > 0 )
		{
			wind_rose_percentage = 1. - (double)( 2.0 * max_size ) / rose_size;
		}
	}

	wind_rose_percentage *= windRoseScale;

	QRect speed_rect = QRect( rectangle.x(), rectangle.y(), rectangle.width(), (int)(wind_speed_percentage*rectangle.height()) );
	QRect rose_rect = QRect( rectangle.x()+(int)(0.5*(rectangle.width()-rose_size)),
	rectangle.y()+(int)(0.5*((1.-wind_speed_percentage)*rectangle.height()-rose_size)+wind_speed_percentage*rectangle.height()),
	rose_size,
	rose_size );

	svg->resize( rose_rect.width() < rose_rect.height() ? rose_rect.width() : rose_rect.height(), rose_rect.width() < rose_rect.height() ? rose_rect.width() : rose_rect.height() );

	QPixmap image = svg->pixmap();

	QSize size;
	size = image.size();
	size.scale( wind_rose_percentage*rose_rect.size(), Qt::KeepAspectRatio );
	image = image.scaled( size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );

	image = image.transformed( transform, Qt::SmoothTransformation );

	QRect image_rect = QRect( rose_rect.x() + (int)(0.5*(rose_rect.width()-image.width())),
								rose_rect.y() + (int)(0.5*(rose_rect.height()-image.height())),
								image.width(),
								image.height() );

	p->drawPixmap( image_rect, image );

	p->setFont( getFittingFontSize( p, speed_rect, Qt::AlignVCenter | Qt::AlignHCenter,
				speed, speed_font, true, true, 0.3, 0, 0 ) );
	drawTextWithShadow( p, speed_rect, Qt::AlignVCenter | Qt::AlignHCenter, speed );

	if ( NSEW.size() >= 4 )
	{
		p->setFont( nsew_new_font );
		drawTextWithShadow( p, rose_rect, Qt::AlignTop | Qt::AlignHCenter, NSEW.at(0) );
		drawTextWithShadow( p, rose_rect, Qt::AlignBottom | Qt::AlignHCenter, NSEW.at(1) );
		drawTextWithShadow( p, rose_rect, Qt::AlignVCenter | Qt::AlignRight, NSEW.at(2) );
		drawTextWithShadow( p, rose_rect, Qt::AlignVCenter | Qt::AlignLeft, NSEW.at(3) );
	}

	delete svg;
	svg = NULL;
}

void PaintHelper::drawGradient( QPainter *p, const QRect &rectangle )
{
	if ( rectangle.width() == 0 || rectangle.height() == 0 ) return;

	QPixmap pixmap(rectangle.size());
	pixmap.fill(Qt::transparent);

	QPainter painter(&pixmap);

	painter.setPen( p->pen() );
	painter.setFont( p->font() );

	QRect rectangle_gradient( QPoint( 0, 0 ), rectangle.size() );

	QPainterPath path;
	path.addRoundRect( rectangle_gradient.adjusted( 0, 0, -1, -1 ), 15 );

	QColor color = QColor( 127,127,127 );
	QColor from = color.darker( 100 );
	QColor to	 = color.lighter( 120 );

	QLinearGradient gradient( 0, 0, 0.75, 1 );
	gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
	gradient.setColorAt( 0, from );
	gradient.setColorAt( 1, to );

	painter.translate( .5, .5 );
	if ( qGray( Plasma::Theme::BackgroundColor ) < 192 )
	{
		painter.setPen( QPen( Qt::white, 1 ) );
	}
	else
	{
		painter.setPen( QPen( Qt::black, 1 ) );
	}
	painter.setBrush( gradient );
	painter.drawPath( path );
	painter.translate( -.5, -.5 );

	gradient.setCoordinateMode( QGradient::ObjectBoundingMode );
	gradient.setColorAt( 0, QColor( 0, 0, 0, 75 ) );
	gradient.setColorAt( 0.6, QColor( 0, 0, 0, 0 ) );
	gradient.setColorAt( 1, QColor( 0, 0, 0, 0 ) );
	painter.setCompositionMode( QPainter::CompositionMode_DestinationIn );
	painter.fillRect( rectangle_gradient, gradient );
	painter.setCompositionMode( QPainter::CompositionMode_SourceOver );

	painter.end();

	p->drawImage( rectangle.topLeft(), pixmap.toImage() );
}

void PaintHelper::paintWeatherSVG( QPainter *p, const QRect &imageRect, const int code, const QString &background_image_file, const bool scale )
{
	QString weather_icon_name_kde;
	QString weather_icon_name_svg;

	// Unused svg weather names:
	//
	// chance-of-fog
	// night-fog
	// cloudy
	// mostly-sunny
	// storm
	// chance-of-snow-showers
	// chance-of-showers
	// chance-of-storm
	// mostly-cloudy-snow
	// chance-of-rain

	switch( code )
	{
		case 31: // 31 Clear/Night
			weather_icon_name_kde = "weather-clear-night";
			weather_icon_name_svg = "night-clear";
			break;
		case 32: // 32 Clear/Day
			weather_icon_name_kde = "weather-clear";
			weather_icon_name_svg = "clear";
			break;
		case 36: // 36 Hot!
			weather_icon_name_kde = "weather-clear";
			weather_icon_name_svg = "hot";
			break;
		case 27: // 27 Mostly Cloudy/Night
			weather_icon_name_kde = "weather-clouds-night";
			weather_icon_name_svg = "night-partly-sunny";
			break;
		case 29: // 29 Partly Cloudy/Night
			weather_icon_name_kde = "weather-clouds-night";
			weather_icon_name_svg = "night-mostly-sunny";
			break;
		case 28: // 28 Mostly Cloudy/Sunny
			weather_icon_name_kde = "weather-clouds";
			weather_icon_name_svg = "partly-sunny";
			break;
		case 33: // 33 Hazy/Night
			weather_icon_name_kde = "weather-few-clouds-night";
			weather_icon_name_svg = "night-haze";
			break;
		case 22: // 22 Smoke
			weather_icon_name_kde = "weather-few-clouds";
			weather_icon_name_svg = "smoke";
			break;
		case 34: // 34 Hazy/Day
			weather_icon_name_kde = "weather-few-clouds";
			weather_icon_name_svg = "haze";
		case 30: // 30 Partly Cloudy/Day
		case 44: // 44 Same as 30
			weather_icon_name_kde = "weather-few-clouds";
			weather_icon_name_svg = "partly-cloudy";
			break;
		case 6: // 06 Hail
			weather_icon_name_kde = "weather-hail";
			weather_icon_name_svg = "snow-showers";
			break;
		case 26: // 26 Mostly Cloudy
			weather_icon_name_kde = "weather-many-clouds.svg";
			weather_icon_name_svg = "mostly-cloudy";
			break;
		case 19: // 19 Dust
			weather_icon_name_kde = "weather-mist";
			weather_icon_name_svg = "dust";
			break;
		case 20: // 20 Fog
			weather_icon_name_kde = "weather-mist";
			weather_icon_name_svg = "fog";
			break;
		case 21: // 21 Haze
			weather_icon_name_kde = "weather-mist";
			weather_icon_name_svg = "haze";
			break;
		case 39: // 39 Rain/Day
			weather_icon_name_kde = "weather-showers-day";
			weather_icon_name_svg = "rain";
			break;
		case 45: // 45 Rain/Night
			weather_icon_name_kde = "weather-showers-night";
			weather_icon_name_svg = "night-chance-of-rain";
			break;
//		 case
//			 weather_icon_name_kde = "weather-showers-scattered-day";
//			 weather_icon_name_svg = "";
//			 break;
//		 case :
//			 weather_icon_name_kde = "weather-showers-scattered-night";
//			 weather_icon_name_svg = "";
//			 break;
		case 8: // 08 Icy/Haze Rain
			weather_icon_name_kde = "weather-showers-scattered";
			weather_icon_name_svg = "icy";
			break;
		case 9: // 09 Haze/Rain
		case 11: // 11 Light Rain
		case 12: // 12 Moderate Rain
			weather_icon_name_kde = "weather-showers-scattered";
			weather_icon_name_svg = "showers";
			break;
		case 10: // 10 Icy/Rain
		case 40: // 40 Rain
			weather_icon_name_kde = "weather-showers";
			weather_icon_name_svg = "rain";
			break;
		case 5: // 05 Cloudy/Snow-Rain Mix
		case 7: // 07 Icy/Clouds Rain-Snow
			weather_icon_name_kde = "weather-snow-rain";
			weather_icon_name_svg = "snow-showers";
			break;
		case 41: // 41 Snow
		case 42: // 42 Same as 41
			weather_icon_name_kde = "weather-snow-scattered-day";
			weather_icon_name_svg = "snow";
			break;
		case 46: // 46 Snow/Night
			weather_icon_name_kde = "weather-snow-scattered-night";
			weather_icon_name_svg = "night-chance-of-snow";
			break;
		case 14: // 14 Same as 13
		case 43: // 43 Windy/Snow
			weather_icon_name_kde = "weather-snow-scattered";
			weather_icon_name_svg = "chance-of-snow";
			break;
		case 13: // 13 Cloudy/Flurries
		case 15: // 15 Flurries
		case 16: // 16 Same as 13
			weather_icon_name_kde = "weather-snow";
			weather_icon_name_svg = "snow";
			break;
		case 37: // 37 Lightning/Day
		case 38: // 38 Lightning
			weather_icon_name_kde = "weather-storm-day";
			weather_icon_name_svg = "chance-of-thunderstorm";
			break;
		case 47: // 47 Thunder Showers/Night
			weather_icon_name_kde = "weather-storm-night";
			weather_icon_name_svg = "night-chance-of-thunderstorm";
			break;
			//		case 0: // 00 Rain/Lightning
		case 1: // 01 Windy/Rain
		case 2: // 02 Same as 01
		case 3: // 03 Same as 00
		case 4: // 04 Same as 00
		case 17: // 17 Same as 00
		case 18: // 18 Same as 00
		case 35: // 35 Same as 00
			weather_icon_name_kde = "weather-storm";
			weather_icon_name_svg = "thunderstorm";
			break;
		case 24: // 24 Same as 23
		case 23: // 23 Windy
			weather_icon_name_kde = "weather-windy";
			weather_icon_name_svg = "windy";
			break;
		case 25: // 25 Frigid
			weather_icon_name_kde = "weather-none-available";
			weather_icon_name_svg = "friged";
			break;
		case 0:
		default:
			weather_icon_name_kde = "weather-none-available";
			weather_icon_name_svg = "unknown";
			break;
	}

	QPixmap weather_icon;

	if ( icons == 2 && iconsCustom.isLocalFile() )
	{
		QRect pixmap_rect = QRect( 0,
									0,
									(int)(( scale ? scaleIcons : 1.0 )*( imageRect.width() > imageRect.height() ? imageRect.height() : imageRect.width() )),
									(int)(( scale ? scaleIcons : 1.0 )*( imageRect.width() > imageRect.height() ? imageRect.height() : imageRect.width() )) );
		weather_icon = QPixmap( pixmap_rect.size() );
		weather_icon.fill( Qt::transparent );

		QPainter painter( &weather_icon );

		Plasma::Svg* svg;
		svg = new Plasma::Svg( this );
		svg->setImagePath( iconsCustom.toLocalFile() );
		svg->setContainsMultipleImages( true );

		svg->resize( pixmap_rect.size() );

		svg->paint( &painter, pixmap_rect, weather_icon_name_svg );

		delete svg;
		svg = NULL;
	}
	else
	{
		weather_icon = KIconLoader::global()->loadIcon( weather_icon_name_kde, KIconLoader::NoGroup,
														(int)(( scale ? scaleIcons : 1.0 )*( imageRect.width() > imageRect.height() ?
														imageRect.height() : imageRect.width() )), KIconLoader::DefaultState, QStringList(), 0L, false );
	}

	double fill_factor = 0.8;

	QRect background_image_rect = imageRect;
	QRect image_rect = QRect( background_image_rect.x() + (int)(0.5*(1.-fill_factor)*background_image_rect.width()),
								background_image_rect.y() + (int)(0.5*(1.-fill_factor)*background_image_rect.height()),
								(int)(fill_factor*background_image_rect.width()),
								(int)(fill_factor*background_image_rect.height()) );

	QSize size;
	size = weather_icon.size();
	size.scale( ( scale ? scaleIcons : 1.0 )*image_rect.size(), Qt::KeepAspectRatio );
	weather_icon = weather_icon.scaled( size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
	image_rect = QRect( imageRect.x() + (int)(0.5*(imageRect.width()-size.width())),
						imageRect.y() + (int)(0.5*(imageRect.height()-size.height())),
						size.width(),
						size.height() ); // Align to center

	if ( !weather_icon.isNull() )
	{
		QImage background_image( background_image_file );
		size = weather_icon.size();
		size.scale( ( scale ? scaleIcons : 1.0 )*background_image_rect.size(), Qt::KeepAspectRatio );
		if ( !background_image.isNull() ) background_image = background_image.scaled( size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
		background_image_rect = QRect( imageRect.x() + (int)(0.5*(imageRect.width()-size.width())),
									imageRect.y() + (int)(0.5*(imageRect.height()-size.height())),
									size.width(),
									size.height() ); // Align to center
		p->drawImage( background_image_rect, background_image );

		p->drawPixmap( image_rect, weather_icon );
	}
}

void PaintHelper::paintWeatherIcon( QPainter *p, const QRect &imageRect, const QImage &img, const QString &background_image_file, const bool scale )
{
	if ( imageRect.width() <= 0 || imageRect.height() <= 0 ) return;

	p->save();

	// Open image files
	QImage image( img );
	if ( image.isNull() ) return;
	QImage background_image( background_image_file );

	double fill_factor = 0.8;

	// original image rects
	QRect background_image_rect = imageRect;
	QRect image_rect = QRect( background_image_rect.x() + (int)(0.5*(1.-fill_factor)*background_image_rect.width()),
								background_image_rect.y() + (int)(0.5*(1.-fill_factor)*background_image_rect.height()),
								(int)(fill_factor*background_image_rect.width()),
								(int)(fill_factor*background_image_rect.height()) );
	// Rescale both rects according to image's aspect ratio
	QSize size;

	size = image.size();
	size.scale( ( scale ? scaleIcons : 1.0 )*image_rect.size(), Qt::KeepAspectRatio );
	image = image.scaled( size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
	image_rect = QRect( imageRect.x() + (int)(0.5*(imageRect.width()-size.width())),
						imageRect.y() + (int)(0.5*(imageRect.height()-size.height())),
						size.width(),
						size.height() ); // Align to center

	size = image.size();
	size.scale( ( scale ? scaleIcons : 1.0 )*background_image_rect.size(), Qt::KeepAspectRatio );
	if ( !background_image.isNull() ) background_image = background_image.scaled( size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
	background_image_rect = QRect( imageRect.x() + (int)(0.5*(imageRect.width()-size.width())),
								imageRect.y() + (int)(0.5*(imageRect.height()-size.height())),
								size.width(),
								size.height() ); // Align to center

	// Draw images
	if ( !background_image.isNull() ) p->drawImage( background_image_rect, background_image );
	p->drawImage( image_rect, image );

	p->restore();
}

void PaintHelper::drawTextWithShadow( QPainter *p, const QRect &rectangle, int flags, const QString &text )
{
	if ( rectangle.width() <= 0 || rectangle.height() <= 0 ) return;

	if(fontShadow)
	{
		QColor shadowColor;
		QPoint shadowOffset;
		QRect rectangle_shadow( QPoint( 0, 0 ), rectangle.size() );

		// Use black shadows with bright text, and white shadows with dark text.
		if (qGray(p->pen().color().rgb()) > 192)
		{
			shadowColor = Qt::black;
		}
		else
		{
			shadowColor = Qt::white;
		}

		// Center white shadows to create a halo effect, and offset dark shadows slightly.
		if (shadowColor == Qt::white)
			shadowOffset = QPoint(0, 0);
		else
			shadowOffset = QPoint(layoutDirection == Qt::RightToLeft ? -1 : 1, 1);

		QPixmap pixmap( rectangle_shadow.size() );
		pixmap.fill(Qt::transparent);

		QPainter p_shadow(&pixmap);

		p_shadow.setPen( p->pen() );
		p_shadow.setPen( shadowColor );
		p_shadow.setFont( p->font() );

		p_shadow.drawText( rectangle_shadow, flags, text );
		p_shadow.end();

		int padding = 2;
		int blurFactor = 3;

		QImage image(rectangle_shadow.size() + QSize(padding * 2, padding * 2), QImage::Format_ARGB32_Premultiplied);
		image.fill(0);
		p_shadow.begin(&image);
		p_shadow.drawImage(padding, padding, pixmap.toImage());
		p_shadow.end();

		Plasma::PaintUtils::shadowBlur(image, blurFactor, shadowColor);

		p->drawImage(rectangle.topLeft() - QPoint(padding, padding) + shadowOffset, image);
		p->drawPixmap(rectangle.topLeft(), pixmap);
	}

	p->drawText( rectangle, flags, text );
}

void PaintHelper::drawLocationImage( QPainter *p, const QRect &rectangle )
{
	if ( rectangle.width() <= 0 || rectangle.height() <= 0 ) return;

	p->save();

	// Open image file
	if ( data_custom_image.isNull() ) return;
	QImage reflection_image = data_custom_image.mirrored( false, true );

	double fill_factor = 0.9;
	double reflection_gap = 0.02;
	double reflection_factor = 0.3;

	// original image rect
	QRect image_rect = QRect( rectangle.x() + (int)(0.5*(1.-fill_factor)*rectangle.width()),
								rectangle.y() + (int)(0.5*(1.-fill_factor)*rectangle.height()),
								(int)(fill_factor*rectangle.width()),
								(int)((1.-reflection_factor)*fill_factor*rectangle.height()) );
	QRect reflection_rect = QRect( image_rect.x(),
								   image_rect.y() + (int)((1.-reflection_factor)*fill_factor*rectangle.height()),
								   (int)(fill_factor*rectangle.width()),
								   (int)(reflection_factor*fill_factor*rectangle.height()) );
	// Rescale rect according to image's aspect ratio
	QSize size;

	size = data_custom_image.size();
	size.scale( image_rect.size(), Qt::KeepAspectRatio );
	QImage image = data_custom_image.scaled( size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
	image_rect = QRect( rectangle.x() + (int)(0.5*(rectangle.width()-size.width())),
						image_rect.y(),
						size.width(),
						size.height() ); // Align to center
	reflection_rect = QRect( image_rect.x(),
							image_rect.y() + (int)((1.+reflection_gap)*size.height()),
							image_rect.width(),
							(int)((1.-reflection_gap)*reflection_rect.height()) );

	reflection_image = blur(reflection_image, 5 );

	// Draw image
	p->drawImage( image_rect, image );

	QPixmap pixmap(reflection_rect.size());
	pixmap.fill(Qt::transparent);

	QPainter painter(&pixmap);

	QRect rectangle_gradient( QPoint( 0, 0 ), reflection_rect.size() );

	painter.drawImage( rectangle_gradient, reflection_image );

	QLinearGradient gradient(0, 0, 0, 1);
	gradient.setCoordinateMode(QGradient::ObjectBoundingMode);

	gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
	gradient.setColorAt(0, QColor(0, 0, 0, 127));
	gradient.setColorAt(0.5, QColor(0, 0, 0, 92));
	gradient.setColorAt(1, QColor(0, 0, 0, 0));
	painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
	painter.fillRect( rectangle_gradient, gradient );
	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

	painter.end();

	p->drawImage( reflection_rect.topLeft(), pixmap.toImage() );

	p->restore();
}

void PaintHelper::drawPlasmaBackground( QPainter *p, const QRect &rectangle, bool translucent )
{
	if ( rectangle.width() == 0 || rectangle.height() == 0 ) return;

	Plasma::Svg* svg;
	svg = new Plasma::Svg( this );

	if ( ( background == 1 || background == 2 || background == 3 ) && translucent ) // TranslucentBackground
		svg->setImagePath( "widgets/translucentbackground" );
	else
		svg->setImagePath( "widgets/background" );

	svg->setContainsMultipleImages( true );

	svg->resize( rectangle.size() );

	int size = 8;

	QRect top_left_rect = QRect( rectangle.x(), rectangle.y(), size, size );
	QRect top_rect = QRect( rectangle.x() + size, rectangle.y(), rectangle.width() - 2 * size, size );
	QRect top_right_rect = QRect( rectangle.x() + rectangle.width() - size, rectangle.y(), size, size );
	QRect left_rect = QRect( rectangle.x(), rectangle.y() + size, size, rectangle.height() - 2 * size );
	QRect center_rect = QRect( rectangle.x() + size, rectangle.y() + size, rectangle.width() - 2 * size, rectangle.height() - 2 * size );
	QRect right_rect = QRect( rectangle.x() + rectangle.width() - size, rectangle.y() + size, size, rectangle.height() - 2 * size );
	QRect bottom_left_rect = QRect( rectangle.x(), rectangle.y() + rectangle.height() - size, size, size );
	QRect bottom_rect = QRect( rectangle.x() + size, rectangle.y() + rectangle.height() - size, rectangle.width() - 2 * size, size );
	QRect bottom_right_rect = QRect( rectangle.x() + rectangle.width() - size, rectangle.y() + rectangle.height() - size, size, size );

	svg->paint( p, top_left_rect, "topleft" );
	svg->paint( p, top_rect, "top" );
	svg->paint( p, top_right_rect, "topright" );

	svg->paint( p, left_rect, "left" );
	svg->paint( p, center_rect, "center" );
	svg->paint( p, right_rect, "right" );

	svg->paint( p, bottom_left_rect, "bottomleft" );
	svg->paint( p, bottom_rect, "bottom" );
	svg->paint( p, bottom_right_rect, "bottomright" );

	delete svg;
	svg = NULL;
}

void PaintHelper::drawExtendedWeatherInformation( QPainter *p, const QRect &rectangle )
{
	if ( rectangle.width() == 0 || rectangle.height() == 0 ) return;

	double line_spacing = 0.3;
	double text_border = 0.0;
	// set y to same offset as x !
	QRect image_rect = QRect( rectangle.x() + (int)(0.5*text_border*rectangle.width()),
							  rectangle.y() + (int)(0.5*text_border*rectangle.width()),
							  (int)((1.-text_border)*rectangle.width()),
							  (int)((1.-text_border)*rectangle.height()) );

	int y_offset = 0;
	if ( data_current_humidity != "" )
	{
		QString text = "";
		if ( humidity != "" ) text = text + humidity + ": ";
		text = text + data_current_humidity;
		drawTextWithShadow( p, QRect( image_rect.x(), image_rect.y() + y_offset, image_rect.width(), image_rect.height() ),
			Qt::AlignTop | Qt::AlignLeft, text );
		y_offset += (int)((line_spacing+1.)*p->boundingRect( rectangle, Qt::AlignTop | Qt::AlignLeft, text ).height());
	}

	if ( data_current_wind != "" )
	{
		QString text = "";
		if ( wind != "" ) text = text + wind + ": ";
		text = text + data_current_wind;
		drawTextWithShadow( p, QRect( image_rect.x(), image_rect.y() + y_offset, image_rect.width(), image_rect.height() ),
			Qt::AlignTop | Qt::AlignLeft, text );
		y_offset += (int)((line_spacing+1.)*p->boundingRect( rectangle, Qt::AlignTop | Qt::AlignLeft, text ).height());
	}

	if ( data_current_rain != "" )
	{
		QString text = "";
		if ( rain != "" ) text = text + rain + ": ";
		text = text + data_current_rain;
		drawTextWithShadow( p, QRect( image_rect.x(), image_rect.y() + y_offset, image_rect.width(), image_rect.height() ),
			Qt::AlignTop | Qt::AlignLeft, text );
		y_offset += (int)((line_spacing+1.)*p->boundingRect( rectangle, Qt::AlignTop | Qt::AlignLeft, text ).height());
	}

	if ( data_current_dew_point != "" && data_current_dew_point.contains( QRegExp( "[0-9]" ) ) )
	{
		QString text = "";
		if ( dewPoint != "" ) text = text + dewPoint + ": ";
		text = text + data_current_dew_point + QChar(0xB0) + tempType;
		drawTextWithShadow( p, QRect( image_rect.x(), image_rect.y() + y_offset, image_rect.width(), image_rect.height() ),
			Qt::AlignTop | Qt::AlignLeft, text );
		y_offset += (int)((line_spacing+1.)*p->boundingRect( rectangle, Qt::AlignTop | Qt::AlignLeft, text ).height());
	}

	if ( data_current_visibility != "" )
	{
		QString text = "";
		if ( visibility != "" ) text = text + visibility + ": ";
		text = text + data_current_visibility;
		drawTextWithShadow( p, QRect( image_rect.x(), image_rect.y() + y_offset, image_rect.width(), image_rect.height() ),
			Qt::AlignTop | Qt::AlignLeft, text );
		y_offset += (int)((line_spacing+1.)*p->boundingRect( rectangle, Qt::AlignTop | Qt::AlignLeft, text ).height());
	}

	if ( data_current_pressure != "" )
	{
		QString text = "";
		if ( pressure != "" ) text = text + pressure + ": ";
		text = text + data_current_pressure;
		drawTextWithShadow( p, QRect( image_rect.x(), image_rect.y() + y_offset, image_rect.width(), image_rect.height() ),
			Qt::AlignTop | Qt::AlignLeft, text );
		y_offset += (int)((line_spacing+1.)*p->boundingRect( rectangle, Qt::AlignTop | Qt::AlignLeft, text ).height());
	}

	if ( data_current_uv_index != "" )
	{
		QString text = "";
		if ( uvIndex != "" ) text = text + uvIndex + ": ";
		text = text + data_current_uv_index;
		drawTextWithShadow( p, QRect( image_rect.x(), image_rect.y() + y_offset, image_rect.width(), image_rect.height() ),
			Qt::AlignTop | Qt::AlignLeft, text );
		y_offset += (int)((line_spacing+1.)*p->boundingRect( rectangle, Qt::AlignTop | Qt::AlignLeft, text ).height());
	}
}

QRect PaintHelper::getExtendedWeatherInformationSize( QPainter *p, const QRect &rectangle )
{
	int height = 0;
	int width = 0;

	double line_spacing = 0.3;
	double text_border = 0.0;
	// set y to same offset as x !
	QRect image_rect = QRect( rectangle.x() + (int)(0.5*text_border*rectangle.width()),
							  rectangle.y() + (int)(0.5*text_border*rectangle.width()),
							  (int)((1.-text_border)*rectangle.width()),
							  (int)((1.-text_border)*rectangle.height()) );

	QRect tmp_rect( 0, 0, 0, 0 );

	int y_offset = 0;
	int absolute_spacing = 0;
	if ( data_current_humidity != "" )
	{
		QString text = "";
		if ( humidity != "" ) text = text + humidity + ": ";
		text = text + data_current_humidity;
		tmp_rect = p->boundingRect( QRect( image_rect.x(), image_rect.y() + y_offset, image_rect.width(), image_rect.height() ),
			Qt::AlignTop | Qt::AlignLeft, text );
		y_offset += (int)((line_spacing+1.)*p->boundingRect( rectangle, Qt::AlignTop | Qt::AlignLeft, text ).height());
		absolute_spacing = (int)(line_spacing*p->boundingRect( rectangle, Qt::AlignTop | Qt::AlignLeft, text ).height());
	}
	if ( tmp_rect.width() > width ) width = tmp_rect.width();

	if ( data_current_wind != "" )
	{
		QString text = "";
		if ( wind != "" ) text = text + wind + ": ";
		text = text + data_current_wind;
		tmp_rect = p->boundingRect( QRect( image_rect.x(), image_rect.y() + y_offset, image_rect.width(), image_rect.height() ),
			Qt::AlignTop | Qt::AlignLeft, text );
		y_offset += (int)((line_spacing+1.)*p->boundingRect( rectangle, Qt::AlignTop | Qt::AlignLeft, text ).height());
		absolute_spacing = (int)(line_spacing*p->boundingRect( rectangle, Qt::AlignTop | Qt::AlignLeft, text ).height());
	}
	if ( tmp_rect.width() > width ) width = tmp_rect.width();

	if ( data_current_rain != "" )
	{
		QString text = "";
		if ( rain != "" ) text = text + rain + ": ";
		text = text + data_current_rain;
		tmp_rect = p->boundingRect( QRect( image_rect.x(), image_rect.y() + y_offset, image_rect.width(), image_rect.height() ),
			Qt::AlignTop | Qt::AlignLeft, text );
		y_offset += (int)((line_spacing+1.)*p->boundingRect( rectangle, Qt::AlignTop | Qt::AlignLeft, text ).height());
		absolute_spacing = (int)(line_spacing*p->boundingRect( rectangle, Qt::AlignTop | Qt::AlignLeft, text ).height());
	}
	if ( tmp_rect.width() > width ) width = tmp_rect.width();

	if ( data_current_dew_point != "" && data_current_dew_point.contains( QRegExp( "[0-9]" ) ) )
	{
		QString text = "";
		if ( dewPoint != "" ) text = text + dewPoint + ": ";
		text = text + data_current_dew_point + QChar(0xB0) + tempType;
		tmp_rect = p->boundingRect( QRect( image_rect.x(), image_rect.y() + y_offset, image_rect.width(), image_rect.height() ),
			Qt::AlignTop | Qt::AlignLeft, text );
		y_offset += (int)((line_spacing+1.)*p->boundingRect( rectangle, Qt::AlignTop | Qt::AlignLeft, text ).height());
		absolute_spacing = (int)(line_spacing*p->boundingRect( rectangle, Qt::AlignTop | Qt::AlignLeft, text ).height());
	}
	if ( tmp_rect.width() > width ) width = tmp_rect.width();

	if ( data_current_visibility != "" )
	{
		QString text = "";
		if ( visibility != "" ) text = text + visibility + ": ";
		text = text + data_current_visibility;
		tmp_rect = p->boundingRect( QRect( image_rect.x(), image_rect.y() + y_offset, image_rect.width(), image_rect.height() ),
			Qt::AlignTop | Qt::AlignLeft, text );
		y_offset += (int)((line_spacing+1.)*p->boundingRect( rectangle, Qt::AlignTop | Qt::AlignLeft, text ).height());
		absolute_spacing = (int)(line_spacing*p->boundingRect( rectangle, Qt::AlignTop | Qt::AlignLeft, text ).height());
	}
	if ( tmp_rect.width() > width ) width = tmp_rect.width();

	if ( data_current_pressure != "" )
	{
		QString text = "";
		if ( pressure != "" ) text = text + pressure + ": ";
		text = text + data_current_pressure;
		tmp_rect = p->boundingRect( QRect( image_rect.x(), image_rect.y() + y_offset, image_rect.width(), image_rect.height() ),
			Qt::AlignTop | Qt::AlignLeft, text );
		y_offset += (int)((line_spacing+1.)*p->boundingRect( rectangle, Qt::AlignTop | Qt::AlignLeft, text ).height());
		absolute_spacing = (int)(line_spacing*p->boundingRect( rectangle, Qt::AlignTop | Qt::AlignLeft, text ).height());
	}
	if ( tmp_rect.width() > width ) width = tmp_rect.width();

	if ( data_current_uv_index != "" )
	{
		QString text = "";
		if ( uvIndex != "" ) text = text + uvIndex + ": ";
		text = text + data_current_uv_index;
		tmp_rect = p->boundingRect( QRect( image_rect.x(), image_rect.y() + y_offset, image_rect.width(), image_rect.height() ),
			Qt::AlignTop | Qt::AlignLeft, text );
		y_offset += (int)((line_spacing+1.)*p->boundingRect( rectangle, Qt::AlignTop | Qt::AlignLeft, text ).height());
		absolute_spacing = (int)(line_spacing*p->boundingRect( rectangle, Qt::AlignTop | Qt::AlignLeft, text ).height());
	}
	if ( tmp_rect.width() > width ) width = tmp_rect.width();

	height = y_offset - absolute_spacing; // no space below text, so remove one time the absolute line spacing

	return QRect( 0, 0, (int)( ( 1. + 2 * text_border ) * width ), (int)( ( 1. + 2 * text_border ) * height ) );
}

QFont PaintHelper::getFittingFontSize( QPainter *p, const QRect &rectangle, int flags, const QString &text, const QFont &original_font,
						  bool minimize_to_width, bool minimize_to_height, double minimal_relative_size, int *width, int *height )
{
	p->save();

	QFont new_font = original_font;

	double reduce_factor = 1.05;
	QRect bounding_rect;
	do
	{
		reduce_factor -= 0.05;
		new_font.setPointSize( (int)( reduce_factor * original_font.pointSize() ) );
		p->setFont( new_font );
		bounding_rect = p->boundingRect( rectangle, flags, text );
	}
	while ( ( ( bounding_rect.width() > rectangle.width() && minimize_to_width ) ||
			  ( bounding_rect.height() > rectangle.height() && minimize_to_height ) ) &&
			reduce_factor > minimal_relative_size );

	if ( width != NULL ) *width = bounding_rect.width();
	if ( height != NULL ) *height = bounding_rect.height();

	p->restore();

	return new_font;
}

QImage PaintHelper::blur( QImage &img, int radius )
{
	QRgb *p1, *p2;
	int x, y, w, h, mx, my, mw, mh, mt, xx, yy;
	int a, r, g, b;
	int *as, *rs, *gs, *bs;
	QRgb p;
	unsigned int a_tmp;
	unsigned int t_tmp;

	if(radius < 1 || img.isNull() || img.width() < (radius << 1))
		return(img);

	w = img.width();
	h = img.height();

	if(img.depth() < 8)
		img = img.convertToFormat(QImage::Format_Indexed8);
	QImage buffer(w, h, img.hasAlphaChannel() ?
	QImage::Format_ARGB32 : QImage::Format_RGB32);

	as = new int[w];
	rs = new int[w];
	gs = new int[w];
	bs = new int[w];

	QVector<QRgb> colorTable;
	if(img.format() == QImage::Format_Indexed8)
		colorTable = img.colorTable();

	for(y = 0; y < h; y++){
		my = y - radius;
		mh = (radius << 1) + 1;
		if(my < 0){
			mh += my;
			my = 0;
		}
		if((my + mh) > h)
			mh = h - my;

		p1 = (QRgb *)buffer.scanLine(y);
		memset(as, 0, w * sizeof(int));
		memset(rs, 0, w * sizeof(int));
		memset(gs, 0, w * sizeof(int));
		memset(bs, 0, w * sizeof(int));

		if(img.format() == QImage::Format_ARGB32_Premultiplied){
			QRgb pixel;
			for(yy = 0; yy < mh; yy++){
				p2 = (QRgb *)img.scanLine(yy + my);
				for(x = 0; x < w; x++, p2++){
					p = *p2;

					a_tmp = p >> 24;
					t_tmp = (p & 0xff00ff) * a_tmp;
					t_tmp = (t_tmp + ((t_tmp >> 8) & 0xff00ff) + 0x800080) >> 8;
					t_tmp &= 0xff00ff;

					p = ((p >> 8) & 0xff) * a_tmp;
					p = (p + ((p >> 8) & 0xff) + 0x80);
					p &= 0xff00;
					p |= t_tmp | (a_tmp << 24);

					pixel = p;
					as[x] += qAlpha(pixel);
					rs[x] += qRed(pixel);
					gs[x] += qGreen(pixel);
					bs[x] += qBlue(pixel);
				}
			}
		}
		else if(img.format() == QImage::Format_Indexed8){
			QRgb pixel;
			unsigned char *ptr;
			for(yy = 0; yy < mh; yy++){
				ptr = (unsigned char *)img.scanLine(yy + my);
				for(x = 0; x < w; x++, ptr++){
					pixel = colorTable[*ptr];
					as[x] += qAlpha(pixel);
					rs[x] += qRed(pixel);
					gs[x] += qGreen(pixel);
					bs[x] += qBlue(pixel);
				}
			}
		}
		else{
			for(yy = 0; yy < mh; yy++){
				p2 = (QRgb *)img.scanLine(yy + my);
				for(x = 0; x < w; x++, p2++){
					as[x] += qAlpha(*p2);
					rs[x] += qRed(*p2);
					gs[x] += qGreen(*p2);
					bs[x] += qBlue(*p2);
				}
			}
		}

		for(x = 0; x < w; x++){
			a = r = g = b = 0;
			mx = x - radius;
			mw = (radius << 1) + 1;
			if(mx < 0){
				mw += mx;
				mx = 0;
			}
			if((mx + mw) > w)
				mw = w - mx;
			mt = mw * mh;
			for(xx = mx; xx < (mw + mx); xx++){
				a += as[xx];
				r += rs[xx];
				g += gs[xx];
				b += bs[xx];
			}
			a = a / mt;
			r = r / mt;
			g = g / mt;
			b = b / mt;
			*p1++ = qRgba(r, g, b, a);
		}
	}
	delete[] as;
	delete[] rs;
	delete[] gs;
	delete[] bs;

	return(buffer);
}

#include "plasma-cwp-paint-helper.moc"
