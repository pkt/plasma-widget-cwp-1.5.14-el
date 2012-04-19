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

#ifndef Plasma_CWP_HEADER
#define Plasma_CWP_HEADER

#include <Plasma/Svg>
#include <QDir>
#include <QTimer>
#include <KConfigDialog>
#include <QWidget>

#include "plasma-cwp-popup-applet.h"
#include "plasma-cwp-paint-helper.h"
#include "plasma-cwp-graphics-widget.h"

#include "ui_config.h"
#include "config.h"

enum LAYOUT_NUMBER
{
	DAYS_1_HORIZONTAL = 0,
	DAYS_1_VERTICAL,
	DAYS_3_HORIZONTAL,
	DAYS_3_VERTICAL,
	DAYS_5_HORIZONTAL,
	DAYS_5_HORIZONTAL_COMPACT,
	DAYS_5_VERTICAL,
	DAYS_7_HORIZONTAL,
	DAYS_7_HORIZONTAL_COMPACT,
	DAYS_7_VERTICAL,
	DAYS_4_CUSTOM_IMAGE_HORIZONTAL,
	DAYS_4_CUSTOM_IMAGE_VERTICAL,
	DAYS_7_CUSTOM_IMAGE
};

struct DataFile
{
	int index;
	QString name;
	QString version;
	QString search_page;
	QString example_zip;
	QString unit;

	QString file_name;
};

struct SatelliteImage
{
	int index;
	QString name;
	QString version;
	QString url;
};

struct PreferredLocation
{
	int index;
	QString name;

	int weather_provider;
	QString zip;

	QStringList custom_image_list;
	QStringList custom_image_name_list;
};

class QSizeF;
class QTimer;

class Data_Provider;

class PixmapDialog;
class PixmapListDialog;

class Plasma_CWP : public CWPPopupApplet
{
	Q_OBJECT

	public:
		// Basic Create/Destroy
		Plasma_CWP( QObject *parent, const QVariantList &args );
		~Plasma_CWP();

		virtual QGraphicsWidget* graphicsWidget();

		void init();
		QSizeF contentSizeHint() const;
		Qt::Orientations expandingDirections() const;
		virtual QList<QAction*> contextualActions();

	signals:
		void reloadRequested();
		void refreshRequested();

	public slots:
		void reloadData();

		void updateLocationImageDialog();
		void updateExtendedWeatherInformationDialog();
		void updatePopupIcon();
		void updateTooltip();
		void updateGraphicsWidgetDelayed();
		void updateGraphicsWidget();

		void refresh();
		void configAppearanceChanged();
		void configAccepted();
		void configRejected();
		void xmlDataFileSelected( int index );
		void zipTextChanged( const QString &text );

		void importSettings();
		void exportSettings();

		void satelliteImagePushButtonPressed();

		void loadLink();

		void preferredLocationActionSelected0() { preferredLocationActionSelected( 0 ); }
		void preferredLocationActionSelected1() { preferredLocationActionSelected( 1 ); }
		void preferredLocationActionSelected2() { preferredLocationActionSelected( 2 ); }
		void preferredLocationActionSelected3() { preferredLocationActionSelected( 3 ); }
		void preferredLocationActionSelected4() { preferredLocationActionSelected( 4 ); }
		void preferredLocationActionSelected5() { preferredLocationActionSelected( 5 ); }
		void preferredLocationActionSelected6() { preferredLocationActionSelected( 6 ); }
		void preferredLocationActionSelected7() { preferredLocationActionSelected( 7 ); }
		void preferredLocationActionSelected8() { preferredLocationActionSelected( 8 ); }
		void preferredLocationActionSelected9() { preferredLocationActionSelected( 9 ); }
		void preferredLocationActionSelected10() { preferredLocationActionSelected( 10 ); }
		void preferredLocationActionSelected11() { preferredLocationActionSelected( 11 ); }
		void preferredLocationActionSelected12() { preferredLocationActionSelected( 12 ); }
		void preferredLocationActionSelected13() { preferredLocationActionSelected( 13 ); }
		void preferredLocationActionSelected14() { preferredLocationActionSelected( 14 ); }
		void preferredLocationActionSelected15() { preferredLocationActionSelected( 15 ); }
		void preferredLocationActionSelected16() { preferredLocationActionSelected( 16 ); }
		void preferredLocationActionSelected17() { preferredLocationActionSelected( 17 ); }
		void preferredLocationActionSelected18() { preferredLocationActionSelected( 18 ); }
		void preferredLocationActionSelected19() { preferredLocationActionSelected( 19 ); }
		void preferredLocationActionSelected20() { preferredLocationActionSelected( 20 ); }
		void preferredLocationActionSelected21() { preferredLocationActionSelected( 21 ); }
		void preferredLocationActionSelected22() { preferredLocationActionSelected( 22 ); }
		void preferredLocationActionSelected23() { preferredLocationActionSelected( 23 ); }
		void preferredLocationActionSelected24() { preferredLocationActionSelected( 24 ); }
		void preferredLocationActionSelected( int index );
		void preferredLocationSelected( int index );
		void preferredLocationNew();
		void preferredLocationSave();
		void preferredLocationEditName();
		void preferredLocationRemove();

		void currentPixmapChanged( int pixmap );
		void customImageUrlChanged( const QString &url );
		void customImageListSelected( int index );
		void customImageNew();
		void customImageSave();
		void customImageEditName();
		void customImageRemove();

		void customImageDialogHidden();
		void mousePressEvent( QGraphicsSceneMouseEvent *event );

	protected:
		void createConfigurationInterface( KConfigDialog *parent );

	private:
		void readConfigData();

		double getFontScale();

		void populateXmlDataFileList( const QDir &dir );
		QList<DataFile> xml_data_file_list;

		void populateBackgroundFileList( const QDir &dir );
		QList<DataFile> background_file_list;

		void populateSatelliteImagesList( const QDir &dir );
		QList<SatelliteImage> satellite_images_list;

		QRect current_weather_rect;
		QRect extended_weather_information_rect;
		bool isInsideCurrentWeather( const QPointF &p );
		void setCurrentWeatherRect( const QRect &rect1, const QRect &rect2, const QRect &rect3, const QRect &rect4, const QRect &rect5 );

		QRect custom_image_full_rect;
		bool isInsideLocationImage( const QPointF &p );
		void setLocationImageRect( const QRect &rect );

		QRect update_time_rect;
		bool provider_update_time_shown;
		bool isInsideUpdateTime( const QPointF &p );
		void setUpdateTimeRect( const QRect &rect );

		CWP_QGraphicsWidget *graphics_widget;
		Data_Provider *data_provider;
		QTimer *update_timer;

		PixmapListDialog *custom_image_dialog;
		PixmapDialog *extended_weather_information_dialog;

		QList<QAction *> actions;
		void createMenu();

		PaintHelper *paint_helper;

		QString provider_url;
		bool last_download_succeeded;

		bool updating;
		bool update_requested;

		bool allow_config_saving;

		uint cwpIdentifier;
		QString updateFrequency;
		int xmlDataFile;
		int backgroundFile;
		QString zip;
		int preferredLocation;
		QList<PreferredLocation> preferred_location_list;
		QString feelsLike;
		QString humidity;
		QString wind;
		QString windRose;
		bool invertWindRose;
		double windRoseScale;
		QString rain;
		QString dewPoint;
		QString visibility;
		QString pressure;
		QString uvIndex;
		QString tempType;
		bool updateTime;
		bool showTemperatureUnit;
		int layoutNumber;
		int icons;
		KUrl iconsCustom;
		int background;
		QColor backgroundColor;
		int backgroundTransparency;
		double scaleIcons;
		QStringList customImageList;
		QStringList customImageNameList;
		int customImageCurrent;
		QString font;
		QColor fontColor;
		bool fontShadow;
		double scaleFont;
		int forecastSeparator;
		bool omitIconDescription;
		int dayNamesSystem;
		bool windLayoutRose;

		QSizeF layoutSizeBeforeConfig;
		QSizeF layoutSizeBeforeConfigGraphicsWidget;

		int default_size;

		QString data_provider_update_time;

		QList<QByteArray> data_custom_image_list;

		QString data_update_time;

		QString data_location_location;
		QString data_location_country;

		QString data_sun_sunrise;
		QString data_sun_sunset;

		QString data_current_temperature;
		QString data_current_temperature_felt;
		QString data_current_wind_code;
		QString data_current_wind_speed;
		QString data_current_wind;
		QString data_current_humidity;
		int data_current_icon_code;
		QImage data_current_icon;
		QString data_current_icon_text;

		QString data_current_rain;
		QString data_current_dew_point;
		QString data_current_visibility;
		QString data_current_pressure;
		QString data_current_uv_index;

		QString data_day_name[7];
		QString data_day_temperature_high[7];
		QString data_day_temperature_low[7];
		int data_day_icon_code[7];
		QImage data_day_icon[7];
		QString data_day_icon_text[7];

		ConfigDialog* conf;
};

// This is the command that links your applet to the .desktop file
K_EXPORT_PLASMA_APPLET(cwp, Plasma_CWP)
#endif
