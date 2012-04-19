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

#ifndef Plasma_CWP_Paint_Helper_HEADER
#define Plasma_CWP_Paint_Helper_HEADER

#include <Plasma/PopupApplet>
#include <QObject>

class PaintHelper : public QObject
{
	Q_OBJECT
	
	public:
		PaintHelper( Plasma::PopupApplet *par );
		~PaintHelper();

		void setValues(
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
		);

		void getRectCurrentHoriz( const int x_0, const int y_0, const int width, const int height, const double x_divisor, const double y_divisor, const double x_start, const double y_start, QRect &complete_rect, QRect &temp_rect, QRect &icon_rect, QRect &icon_text_rect, QRect &wind_rect, QRect &sun_rect );
		void getRectCurrentVert( const int x_0, const int y_0, const int width, const int height, const double x_divisor, const double y_divisor, const double x_start, const double y_start, QRect &complete_rect, QRect &temp_rect, QRect &icon_rect, QRect &icon_text_rect, QRect &wind_rect, QRect &sun_rect );
		void getRectCurrentLoc( const int x_0, const int y_0, const int width, const int height, const double x_divisor, const double y_divisor, const double x_start, const double y_start, QRect &complete_rect, QRect &temp_rect, QRect &wind_rect, QRect &sun_rect );
		void getRectLocationImage( const int x_0, const int y_0, const int width, const int height, const double x_divisor, const double y_divisor, const double x_start, const double y_start, QRect &custom_image_rect, QRect &icon_rect, QRect &icon_text_rect, QRect &title_rect, QRect &clock_rect );
		void getRectForecast( const int x_0, const int y_0, const int width, const int height, const double x_divisor, double y_divisor, const double x_start, const double y_start, QRect &complete_rect, QRect &name_rect, QRect &temp_rect, QRect &icon_rect, QRect &icon_text_rect );
		
		void drawBackground( QPainter *p, QRect &contents_rect );
		void drawGradient( QPainter *p, const QRect &rectangle );
		void drawWind( QPainter *p, const QRect &rectangle, const QString &code, const QString &speed, const QString &title, const QString &rose_text, const QFont &speed_font, const QFont &nsew_font );
		void paintWeatherSVG( QPainter *p, const QRect &imageRect, const int code, const QString &background_image_file, const bool scale );
		void paintWeatherIcon( QPainter *p, const QRect &imageRect, const QImage &img, const QString &background_image_file, const bool scale );
		void drawTextWithShadow( QPainter *p, const QRect &rectangle, int flags, const QString &text );
		void drawLocationImage( QPainter *p, const QRect &rectangle );
		void drawPlasmaBackground( QPainter *p, const QRect &rectangle, bool translucent );
		void drawExtendedWeatherInformation( QPainter *p, const QRect &rectangle );
		QRect getExtendedWeatherInformationSize( QPainter *p, const QRect &rectangle );
		QFont getFittingFontSize( QPainter *p, const QRect &rectangle, int flags, const QString &text, const QFont &original_font, bool minimize_to_width = true, bool minimize_to_height = true, double minimal_relative_size = 0.5, int *width = 0, int *height = 0 );

	private:
		Plasma::PopupApplet *parent;
		QImage blur( QImage &img, int radius );

		QString humidity;
		QString wind;
		bool invertWindRose;
		double windRoseScale;
		QString rain;
		QString dewPoint;
		QString visibility;
		QString pressure;
		QString uvIndex;
		QString tempType;
		int icons;
		KUrl iconsCustom;
		int background;
		QColor backgroundColor;
		int backgroundTransparency;
		double scaleIcons;
		QString font;
		QColor fontColor;
		bool fontShadow;
		double scaleFont;
		int forecastSeparator;

		int layoutDirection;

		QImage data_custom_image;

		QString data_current_wind;
		QString data_current_humidity;

		QString data_current_rain;
		QString data_current_dew_point;
		QString data_current_visibility;
		QString data_current_pressure;
		QString data_current_uv_index;
};

#endif
