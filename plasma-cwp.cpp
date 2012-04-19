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

#include <QGraphicsSceneMouseEvent>
#include <QInputDialog>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <KGlobalSettings>
#include <QDesktopServices>
#include <QDomDocument>
#include <QDir>
#include <KConfigGroup>

#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>

#include <Plasma/ToolTipManager>
#include <Plasma/PaintUtils>
#include <Plasma/Svg>

#include <math.h>

#include "plasma-cwp.h"
#include "plasma-cwp-data-provider.h"
#include "plasma-cwp-pixmap-dialog.h"
#include "plasma-cwp-pixmap-list-dialog.h"
#include "config.h"

Plasma_CWP::Plasma_CWP( QObject *parent, const QVariantList &args )
	: CWPPopupApplet( parent, args )
{
	data_provider = NULL;
	tempType = "";
	data_update_time = "";

	conf = NULL;

	last_download_succeeded = false;

	updating = false;
	update_requested = false;

	allow_config_saving = true;

	setHasConfigurationInterface( true );
}

Plasma_CWP::~Plasma_CWP()
{
// 	update_timer->stop();
	allow_config_saving = false;

// 	if ( custom_image_dialog ) custom_image_dialog->close();
// 	delete custom_image_dialog;
// 	custom_image_dialog = NULL;

// 	if ( extended_weather_information_dialog ) extended_weather_information_dialog->close();
// 	delete extended_weather_information_dialog;
// 	extended_weather_information_dialog =  NULL;

// 	delete data_provider;
// 	data_provider = NULL;

// 	delete graphics_widget;
// 	graphics_widget = NULL;
}

void Plasma_CWP::init()
{
	paint_helper = new PaintHelper( this );

	graphics_widget = new CWP_QGraphicsWidget( this );
	graphics_widget->setMinimumSize( QSizeF( 150, 150 ) );

	setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

	setAspectRatioMode( Plasma::IgnoreAspectRatio );

	custom_image_dialog = new PixmapListDialog;
	custom_image_dialog->setResizable( true );
	custom_image_dialog->setWindowFlags( Qt::X11BypassWindowManagerHint );
	extended_weather_information_dialog = new PixmapDialog;
	extended_weather_information_dialog->setResizable( false );
	extended_weather_information_dialog->setWindowFlags( Qt::X11BypassWindowManagerHint );

	connect( custom_image_dialog, SIGNAL( hidden() ), this, SLOT( customImageDialogHidden() ) );
	connect( custom_image_dialog, SIGNAL( currentImageChanged( int ) ), this, SLOT( currentPixmapChanged( int ) ) );

	connect( graphics_widget, SIGNAL( pixmapUpdate() ), this, SLOT( updateGraphicsWidgetDelayed() ) );
	connect( graphics_widget, SIGNAL( mousePressEventReceived( QGraphicsSceneMouseEvent * ) ), this, SLOT( mousePressEvent( QGraphicsSceneMouseEvent * ) ) );

	connect( this, SIGNAL( refreshRequested() ), this, SLOT( refresh() ) );
	connect( this, SIGNAL( reloadRequested() ), this, SLOT( reloadData() ) );

	setBusy( true );

	preferred_location_list.clear();

	xml_data_file_list.clear();
	populateXmlDataFileList( QDir( KGlobal::dirs()->findResourceDir( "data", "plasma-cwp/www_weather_com.xml" ) + "plasma-cwp" ) );
	populateXmlDataFileList( QDir( KStandardDirs::locateLocal( "data", "cwp/" ) ) );

	satellite_images_list.clear();
	populateSatelliteImagesList( QDir( KGlobal::dirs()->findResourceDir( "data", "plasma-cwp/www_weather_com.xml" ) + "plasma-cwp" ) );
	populateSatelliteImagesList( QDir( KStandardDirs::locateLocal( "data", "cwp/" ) ) );

	background_file_list.clear();
	// prepend an empty background file
	DataFile background_file_tmp;
	background_file_tmp.index = 0;
	background_file_tmp.name = i18nc( "No background image for weather icons", "< none >" );
	background_file_tmp.version = "";
	background_file_tmp.search_page = "";
	background_file_tmp.example_zip = "";
	background_file_tmp.file_name = "";
	background_file_list.append( background_file_tmp );
	populateBackgroundFileList( QDir( KGlobal::dirs()->findResourceDir( "data", "plasma-cwp/www_weather_com.xml" ) + "plasma-cwp" ) );
	populateBackgroundFileList( QDir( KStandardDirs::locateLocal( "data", "cwp/" ) ) );

	if ( size().height() < 150. ) Plasma::ToolTipManager::self()->registerWidget( this );

	readConfigData();

	updateGraphicsWidget();
	updateLocationImageDialog();
	updateExtendedWeatherInformationDialog();
	updatePopupIcon();
	updateTooltip();

	createMenu();

	update_timer = new QTimer( this );
	connect( update_timer, SIGNAL( timeout() ), this, SLOT( reloadData() ) );
	bool error;
	update_timer->start( updateFrequency.toInt( &error )*60*1000 ); // update every x minutes

	QTimer::singleShot( 5*1000, this, SLOT( reloadData() ) );

	QTimer::singleShot( 100, this, SLOT( refresh() ) );
}

void Plasma_CWP::refresh()
{
	if ( !conf ) readConfigData();

	QList<KUrl> tmp_url_list;
	int i;
	for ( i=0; i<customImageList.size(); i++ )
	{
		tmp_url_list.append( KUrl( customImageList.at( i ) ) );
	}

	if ( data_provider )
	{
		if ( xmlDataFile < xml_data_file_list.size() && xmlDataFile >= 0 && xml_data_file_list.size() > 0 )
			data_provider->set_config_values( updateFrequency, xml_data_file_list.at(xmlDataFile).file_name, zip,
				feelsLike, humidity, wind, xml_data_file_list.at(xmlDataFile).unit, tmp_url_list );

		data_provider->get_weather_values( data_location_location, data_location_country, data_sun_sunrise,
			data_sun_sunset, data_current_temperature, data_current_temperature_felt, data_current_wind_code, data_current_wind_speed, data_current_wind,
			data_current_humidity, data_current_dew_point, data_current_visibility, data_current_pressure, data_current_uv_index,
			data_current_icon_code, data_current_icon, data_current_icon_text, data_current_rain,
			&data_day_name[0], &data_day_temperature_high[0], &data_day_temperature_low[0], &data_day_icon_code[0], &data_day_icon[0],
			&data_day_icon_text[0], data_update_time, data_provider_update_time, tempType, data_custom_image_list,
			last_download_succeeded, provider_url );

		KConfigGroup cg = config();
		cg.writeEntry( "ProviderURL", provider_url );
		emit configNeedsSaving();
	}

	paint_helper->setValues( humidity, wind, invertWindRose, windRoseScale, rain, dewPoint, visibility,
		pressure, uvIndex, tempType, icons, iconsCustom, background, backgroundColor,
		backgroundTransparency, scaleIcons, font, fontColor, fontShadow, scaleFont,
		forecastSeparator, layoutDirection(), data_custom_image_list.size() > 0 ? data_custom_image_list.at ( 0 ) : QByteArray(),
		data_current_wind, data_current_humidity, data_current_rain, data_current_dew_point,
		data_current_visibility, data_current_pressure, data_current_uv_index );
	custom_image_dialog->setFont( font );

	updateGraphicsWidgetDelayed();
	updateLocationImageDialog();
	updateExtendedWeatherInformationDialog();
	updatePopupIcon();
	update();
	graphics_widget->update();
}

void Plasma_CWP::reloadData()
{
	if ( data_provider )
	{
		data_provider->reloadData();
	}
	else
	{
		data_provider = new Data_Provider( this, KStandardDirs::locateLocal( "data", "cwp/" ), cwpIdentifier );
		connect( data_provider, SIGNAL( data_fetched() ), this, SLOT( refresh() ) );
		QTimer::singleShot( 10*1000, this, SLOT( reloadData() ) );
	}

	QList<KUrl> tmp_url_list;
	int i;
	for ( i=0; i<customImageList.size(); i++ )
	{
		tmp_url_list.append( KUrl( customImageList.at( i ) ) );
	}

	if ( xmlDataFile < xml_data_file_list.size() && xmlDataFile >= 0 && xml_data_file_list.size() > 0 )
		data_provider->set_config_values( updateFrequency, xml_data_file_list.at(xmlDataFile).file_name, zip,
			feelsLike, humidity, wind, xml_data_file_list.at(xmlDataFile).unit, tmp_url_list );

	createMenu();
	refresh();
}

QGraphicsWidget* Plasma_CWP::graphicsWidget()
{
	return graphics_widget;
}

void Plasma_CWP::readConfigData()
{
	KConfigGroup cg = config();

	cwpIdentifier = cg.readEntry( "cwpIdentifier", 0 );
	// Use current time to create a unique hash
	if ( cwpIdentifier == 0 ) cwpIdentifier = qHash( QString( "cwp%1" ).arg( random() ) );
	xmlDataFile = cg.readEntry( "xmlDataFile", 0 );
	zip = cg.readEntry( "zip", "BRXX0043");
	updateFrequency = cg.readEntry( "updateFrequency", "5" );
	updateTime = cg.readEntry( "updateTime", true );
	showTemperatureUnit = cg.readEntry( "showTemperatureUnit", true );

	provider_update_time_shown = cg.readEntry( "provider_update_time_shown", false );

	layoutNumber = cg.readEntry( "layoutNumber", 0 );
	icons = cg.readEntry( "icons", 1 );
	iconsCustom = cg.readEntry( "iconsCustom", "" );
	backgroundFile = cg.readEntry( "backgroundFile", 0 );
	scaleIcons = cg.readEntry( "scaleIcons", 1.3 );
	forecastSeparator = cg.readEntry( "forecastSeparator", 1 );
	customImageList = cg.readEntry( "customImageList", QStringList() );
	customImageNameList = cg.readEntry( "customImageNameList", QStringList() );

	if ( customImageList.size() <= 0 || customImageNameList.size() <= 0 )
	{
		customImageList.clear();
		customImageNameList.clear();
		QString custom_image_pre_0_9_8 = cg.readEntry( "locationImage", "" );
		if ( custom_image_pre_0_9_8 != "" )
		{
			customImageList.append( custom_image_pre_0_9_8 );
			customImageNameList.append( "" );
		}
	}

	customImageCurrent = cg.readEntry( "customImageCurrent", 0 );
	feelsLike = cg.readEntry( "feelsLike", i18nc( "Due to wind chill, temperature is felt like", "feels like" ) );
	humidity = cg.readEntry( "humidity", i18n( "Humidity" ) );
	wind = cg.readEntry( "wind", i18n( "Wind" ) );
	windRose = cg.readEntry( "windRose", i18nc( "Wind directions: North, South, East, West. Separated by \";\", no spaces!", "N;S;E;W;" ) );
	invertWindRose = cg.readEntry( "invertWindRose", false );
	windRoseScale = cg.readEntry( "windRoseScale", 0.9 );
	rain = cg.readEntry( "rain", i18n( "Rain" ) );
	dewPoint = cg.readEntry( "dewPoint", i18n( "Dew Point" ) );
	visibility = cg.readEntry( "visibility", i18n( "Visibility" ) );
	pressure = cg.readEntry( "pressure", i18n( "Pressure" ) );
	uvIndex = cg.readEntry( "uvIndex", i18n( "UV Index" ) );

	background = cg.readEntry( "background", 1 );
	backgroundColor = cg.readEntry( "backgroundColor", Plasma::Theme::defaultTheme()->color( Plasma::Theme::BackgroundColor ) );
	backgroundTransparency = cg.readEntry( "backgroundTransparency", 224 );
	font = cg.readEntry( "font", Plasma::Theme::defaultTheme()->font( Plasma::Theme::DefaultFont ).toString() );
	fontColor = cg.readEntry( "fontColor", Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
	fontShadow = cg.readEntry( "fontShadow", true );
	scaleFont = cg.readEntry( "scaleFont", 1.0 );

	omitIconDescription = cg.readEntry( "omitIconDescription", false );
	dayNamesSystem = cg.readEntry( "dayNamesSystem", 1 );

	windLayoutRose = cg.readEntry( "windLayoutRose", true );

	provider_url = cg.readEntry( "ProviderURL", "" );

	QList<int> index_list = cg.readEntry( "preferredLocationIndexList", QList<int>() );
	QStringList name_list = cg.readEntry( "preferredLocationNameList", QStringList() );
	QList<int> weather_provider_list = cg.readEntry( "preferredLocationWeatherProviderList", QList<int>() );
	QStringList zip_list = cg.readEntry( "preferredLocationZipList", QStringList() );

	int i;
	if ( index_list.size() == name_list.size() && index_list.size() == weather_provider_list.size() && index_list.size() == zip_list.size() )
	{
		preferred_location_list.clear();
		PreferredLocation tmp_location;
		for ( i=0; i<index_list.size(); i++ )
		{
			tmp_location.index = index_list.at(i);
			tmp_location.name = name_list.at(i);
			tmp_location.weather_provider = weather_provider_list.at(i);
			tmp_location.zip = zip_list.at(i);

			preferred_location_list.append( tmp_location );
		}
	}
	for ( i=0; i<25; i++ )
	{
		QStringList custom_image_list = cg.readEntry( QString( "preferredLocationCustomImageList%1" ).arg( i ), QStringList() );
		QStringList custom_image_name_list = cg.readEntry( QString( "preferredLocationCustomImageNameList%1" ).arg( i ), QStringList() );

		if ( custom_image_list.size() == custom_image_name_list.size() && custom_image_list.size() > 0 )
		{
			if ( i < preferred_location_list.size() )
			{
				preferred_location_list[i].custom_image_list = custom_image_list;
				preferred_location_list[i].custom_image_name_list = custom_image_name_list;
			}
		}
	}

	// size used for font scale on tooltip
	QSizeF size_tmp = cg.readEntry( "graphics_widget_size", QSizeF( 0., 0. ) );
	if ( size_tmp.width() >= 150. && size_tmp.height() >= 150. )
	{
		graphics_widget->resize( size_tmp );
		graphics_widget->getPixmap() = graphics_widget->getPixmap().scaled( size_tmp.toSize() );
	}

	QSize custom_image_dialog_size = cg.readEntry( "custom_image_dialog_size", QSize( 0, 0 ) );
	if ( custom_image_dialog_size.width() != 0 && custom_image_dialog_size.height() != 0 )
	{
		custom_image_dialog->resize( custom_image_dialog_size );
	}

	if ( size().height() < 150. )
	{
		setBackgroundHints( Applet::NoBackground );
	}
	else
	{
		if ( background == 1 ) // translucent
			setBackgroundHints( Applet::TranslucentBackground );
		else if ( background == 2 ) // none
			setBackgroundHints( Applet::NoBackground );
		else if ( background == 3 ) // custom
			setBackgroundHints( Applet::NoBackground );
		else
			setBackgroundHints( Applet::DefaultBackground );
	}
}

void Plasma_CWP::updateLocationImageDialog()
{
	if ( data_custom_image_list.size() <= 0 ) return;

	custom_image_dialog->setImageList( data_custom_image_list );
	custom_image_dialog->setNameList( customImageNameList );
	custom_image_dialog->setCurrentImage( customImageCurrent );
}

void Plasma_CWP::updateExtendedWeatherInformationDialog()
{
	double font_scale = getFontScale();

	QColor palette_font_color = Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor );

	QRect tmp_rect( 0, 0, 1024, 1024 );
	QPixmap tmp_pm( tmp_rect.size() );

	QPainter tmp_painter( &tmp_pm );

	tmp_painter.setRenderHints( QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing | QPainter::Antialiasing );
	tmp_painter.setPen( palette_font_color );
	tmp_painter.setFont( QFont( font, (int)(font_scale*scaleFont*9), QFont::Normal ) );

	QSize info_size = paint_helper->getExtendedWeatherInformationSize( &tmp_painter, tmp_rect ).size();

	QPixmap pm( info_size );
	pm.fill( Qt::transparent );

	QPainter painter( &pm );

	painter.setRenderHints( QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing | QPainter::Antialiasing );
	painter.setPen( palette_font_color );
	painter.setFont( QFont( font, (int)(font_scale*scaleFont*9), QFont::Normal ) );

	QRect rect( 0, 0, pm.width(), pm.height() );

	paint_helper->drawExtendedWeatherInformation( &painter, rect );

	extended_weather_information_dialog->setPixmap( pm );
}


void Plasma_CWP::updatePopupIcon()
{
	QRect contents_rect( 0, 0, size().toSize().width(), size().toSize().height() );
	QRect icon_rect( 0, 0, contents_rect.width(), (int)(0.75*contents_rect.height()) );
	QRect text_rect( 0, (int)(0.5*contents_rect.height()), contents_rect.width(), (int)(0.5*contents_rect.height()) );

	if ( text_rect.height() < 12 )
	{
		text_rect = contents_rect;
		icon_rect = QRect( 0, 0, 0, 0 );
	}
	QPixmap pixmap( size().toSize() );
	pixmap.fill( Qt::transparent );

	QPainter painter( &pixmap );

	painter.setRenderHints( QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing | QPainter::Antialiasing );
	painter.setPen( fontColor );

	if ( data_update_time != "" )
	{
		setBusy( false );

		int alignment = Qt::AlignBottom | Qt::AlignHCenter;

		if ( icon_rect.height() > 0 )
		{
			if ( icons > 0 )
			{
				if ( backgroundFile < background_file_list.size() && backgroundFile >= 0 && background_file_list.size() > 0 )
					paint_helper->paintWeatherSVG( &painter, icon_rect, data_current_icon_code, background_file_list.at(backgroundFile).file_name, false );
			}
			else
			{
				if ( backgroundFile < background_file_list.size() && backgroundFile >= 0 && background_file_list.size() > 0 )
					paint_helper->paintWeatherIcon( &painter, icon_rect, data_current_icon, background_file_list.at(backgroundFile).file_name, false );
			}
		}
		else
		{
			alignment = Qt::AlignVCenter | Qt::AlignHCenter;
		}

		QFont temperature_font = paint_helper->getFittingFontSize( &painter, text_rect, alignment,
			data_current_temperature + QChar(0xB0) + ( showTemperatureUnit ? tempType : "" ),
			QFont( font, (int)( size().width() ), QFont::Normal ), true, true, 0.2 );

		painter.setFont( temperature_font );

		paint_helper->drawTextWithShadow( &painter, text_rect, alignment, data_current_temperature + QChar(0xB0) + ( showTemperatureUnit ? tempType : "" ) );
	}

	painter.end();

	setPopupIcon( QIcon( pixmap ) );
}

void Plasma_CWP::updateTooltip()
{
	if ( size().height() >= 150 )
	{
		Plasma::ToolTipManager::self()->unregisterWidget( this );
		return; // Don't draw tooltip for non-systray mode
	}

	if ( data_update_time == "" )
	{
		QRect contents_rect = QRect( 0, 0, 80, 80 );

		Plasma::Svg* svg;
		svg = new Plasma::Svg( this );
		svg->setImagePath( "widgets/busywidget" );
		svg->setContainsMultipleImages( false );

		QSize size;
		size = svg->pixmap().size();
		size.scale( contents_rect.size(), Qt::KeepAspectRatio );

		QRect scaled_rect = QRect( contents_rect.x() + (int)( 0.5 * ( contents_rect.width() - size.width() ) ),
								   contents_rect.y() + (int)( 0.5 * ( contents_rect.height() - size.height() ) ),
								   size.width(),
								   size.height() );

		svg->resize( scaled_rect.size() );

		QPixmap pm( contents_rect.size() );
		pm.fill( Qt::transparent );

		QPainter p( &pm );
		p.setRenderHints( QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing | QPainter::Antialiasing );

		svg->paint( &p, scaled_rect );

		Plasma::ToolTipContent data;
		data.setImage( pm );

		Plasma::ToolTipManager::self()->setContent( this, data );

		delete svg;
		svg = NULL;

		return;
	}

	double font_scale = getFontScale();

	QColor palette_font_color = Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor );

	QRect tmp_rect( 0, 0, 1024, 1024 );

	QPixmap pixmap( tmp_rect.size() );
	pixmap.fill( Qt::transparent );
	QPainter painter( &pixmap );

	painter.setRenderHints( QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing | QPainter::Antialiasing );
	painter.setPen( palette_font_color );

	painter.setFont( QFont( font, (int)(font_scale*scaleFont*9), QFont::Normal ) );
	QRect tooltip_rect = paint_helper->getExtendedWeatherInformationSize( &painter, tmp_rect );

	painter.setFont( QFont( font, (int)(font_scale*scaleFont*11), QFont::Bold ) );
	QString location_country = "";
	if( data_location_location != "" ) location_country = location_country + data_location_location;
	if(	data_location_location != "" && data_location_country != "" ) location_country = location_country + ", ";
	if( data_location_country != "" )	location_country = location_country + data_location_country;
	QRect title_rect = painter.boundingRect( tmp_rect, Qt::AlignHCenter | Qt::AlignTop, location_country );

	title_rect = QRect( 0, 0, title_rect.width(), title_rect.height() );

	int icon_height = 80;
	int separator = 8;
	double icon_percentage = 0.8;
	if ( omitIconDescription ) icon_percentage = 1.0;

	int final_width = title_rect.width();
	if ( tooltip_rect.width() > final_width ) final_width = tooltip_rect.width();

	painter.setFont( QFont( font, (int)(font_scale*scaleFont*24), QFont::Normal ) );
	int temp_width = painter.boundingRect( tmp_rect, Qt::AlignTop | Qt::AlignHCenter,
		data_current_temperature + QChar(0xB0) + ( showTemperatureUnit ? tempType : "" ) ).width();
	if ( 2 * temp_width > final_width ) final_width = 2 * temp_width;

	painter.end();

	title_rect = QRect( 0, 0, final_width, title_rect.height() );

	QRect icon_rect = QRect( 0,
							 title_rect.height(),
							 (int)( 0.5*final_width ),
							 (int)( icon_percentage * icon_height ) );
	QRect icon_text_rect = QRect( 0,
								  (int)( title_rect.height() + 0.5 * icon_percentage * icon_height ),
								  (int)( 0.5 * final_width ),
								  (int)( ( 1.-icon_percentage + 0.5 * icon_percentage ) * icon_height) );
	QRect temperature_rect = QRect( (int)(0.5*final_width),
									title_rect.height(),
									(int)(0.5*final_width),
									icon_height );
	tooltip_rect = QRect( (int)(0.5*(final_width-tooltip_rect.width())),
						  title_rect.height() + icon_height + separator,
						  tooltip_rect.width(),
						  tooltip_rect.height() );

	int final_height = title_rect.height() + tooltip_rect.height() + icon_height + separator;
	QRect total_rect = QRect( 0, 0, final_width, final_height );

	QPixmap real_pixmap( total_rect.size() );
	real_pixmap.fill( Qt::transparent );

	painter.begin( &real_pixmap );

	painter.setRenderHints( QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing | QPainter::Antialiasing );
	painter.setPen( palette_font_color );

	// icon
	if ( icons > 0 )
	{
		if ( backgroundFile < background_file_list.size() && backgroundFile >= 0 && background_file_list.size() > 0 )
			paint_helper->paintWeatherSVG( &painter, icon_rect, data_current_icon_code, background_file_list.at(backgroundFile).file_name, true );
	}
	else
	{
		if ( backgroundFile < background_file_list.size() && backgroundFile >= 0 && background_file_list.size() > 0 )
			paint_helper->paintWeatherIcon( &painter, icon_rect, data_current_icon, background_file_list.at(backgroundFile).file_name, true );
	}

	// location / country
	painter.setFont( paint_helper->getFittingFontSize( &painter, title_rect, Qt::AlignVCenter | Qt::AlignHCenter,
		location_country, QFont( font, (int)(font_scale*scaleFont*11), QFont::Bold ), true, true, 0.3, 0, 0 ) );
	paint_helper->drawTextWithShadow( &painter, title_rect, Qt::AlignVCenter | Qt::AlignHCenter, location_country );

	if ( !omitIconDescription )
	{
		painter.setFont( paint_helper->getFittingFontSize( &painter, icon_text_rect, Qt::AlignBottom | Qt::AlignHCenter | Qt::TextWordWrap,
			data_current_icon_text, QFont( font, (int)(font_scale*scaleFont*9), QFont::Normal ), false, true, 0.3, 0, 0 ) );
		paint_helper->drawTextWithShadow( &painter, icon_text_rect, Qt::AlignBottom | Qt::AlignHCenter | Qt::TextWordWrap, data_current_icon_text );
	}

	// temperature
	int feels_like_height = 0;
	if( data_current_temperature_felt != "" && data_current_temperature_felt.contains(QRegExp("[0-9]")) )
	{
		// maximum height: 0.5 * space
		painter.setFont( paint_helper->getFittingFontSize( &painter, QRect( temperature_rect.x(), temperature_rect.y() + (int)( 0.5 * temperature_rect.height() ),
			temperature_rect.width(), (int)( 0.5 * temperature_rect.height() ) ),
			Qt::AlignBottom | Qt::AlignHCenter, feelsLike + "\n" + data_current_temperature_felt + QChar(0xB0) + ( showTemperatureUnit ? tempType : "" ),
			QFont( font, (int)(font_scale*scaleFont*9), QFont::Normal ), true, true, 0.3, 0, &feels_like_height ) );
		paint_helper->drawTextWithShadow( &painter, temperature_rect, Qt::AlignBottom | Qt::AlignHCenter,
			feelsLike + "\n" + data_current_temperature_felt + QChar(0xB0) + ( showTemperatureUnit ? tempType : "" ) );
	}

	QRect current_temp_remaining_rect( temperature_rect.x(), temperature_rect.y(), temperature_rect.width(), temperature_rect.height() - feels_like_height );

	int temperature_height = 0;
	painter.setFont( paint_helper->getFittingFontSize( &painter, current_temp_remaining_rect, Qt::AlignTop | Qt::AlignHCenter,
		data_current_temperature + QChar(0xB0) + ( showTemperatureUnit ? tempType : "" ), QFont( font, (int)(font_scale*scaleFont*24), QFont::Normal ),
		true, true, 0.3, 0, &temperature_height ) );

	if ( temperature_height > 0 )
	{
		current_temp_remaining_rect = QRect( current_temp_remaining_rect.x(),
			current_temp_remaining_rect.y() + (int)( floor( 0.5 * ( current_temp_remaining_rect.height() - temperature_height ) ) ),
			current_temp_remaining_rect.width(),
			(int)( current_temp_remaining_rect.height() - floor( 0.5 * ( current_temp_remaining_rect.height() - temperature_height ) ) ) );
	}

	paint_helper->drawTextWithShadow( &painter, current_temp_remaining_rect, Qt::AlignTop | Qt::AlignHCenter,
		data_current_temperature + QChar(0xB0) + ( showTemperatureUnit ? tempType : "" ) );

	// Extended weather information
	painter.setFont( QFont( font, (int)(font_scale*scaleFont*9), QFont::Normal ) );
	paint_helper->drawExtendedWeatherInformation( &painter, tooltip_rect );

	painter.end();

	Plasma::ToolTipContent data;
	data.setImage( real_pixmap );

	Plasma::ToolTipManager::self()->setContent( this, data );
}

void Plasma_CWP::updateGraphicsWidgetDelayed()
{
	if ( updating )
	{
		update_requested = true;
	}
	else
	{
		updating = true;
		update_requested = false;
		QTimer::singleShot( 300, this, SLOT( updateGraphicsWidget() ) );
	}
}

void Plasma_CWP::updateGraphicsWidget()
{
	if ( update_requested )
	{
		updating = false;
		update_requested = false;
		updateGraphicsWidgetDelayed();
		return;
	}

	QRect contents_rect = graphics_widget->rect().toRect();
	QPixmap &contents_pixmap = graphics_widget->getPixmap();
	contents_pixmap.fill( Qt::transparent );
	QPainter *p = new QPainter( &contents_pixmap );
	contents_rect= QRect( 0, 0, contents_rect.width(), contents_rect.height() );

	if ( background == 3 ) paint_helper->drawBackground( p, contents_rect );

	if ( data_provider )
	{
		data_provider->get_weather_values( data_location_location, data_location_country, data_sun_sunrise,
			data_sun_sunset, data_current_temperature, data_current_temperature_felt, data_current_wind_code, data_current_wind_speed, data_current_wind,
			data_current_humidity, data_current_dew_point, data_current_visibility, data_current_pressure, data_current_uv_index,
			data_current_icon_code, data_current_icon, data_current_icon_text, data_current_rain,
			&data_day_name[0], &data_day_temperature_high[0], &data_day_temperature_low[0], &data_day_icon_code[0], &data_day_icon[0],
			&data_day_icon_text[0], data_update_time, data_provider_update_time, tempType, data_custom_image_list,
			last_download_succeeded, provider_url );

		KConfigGroup cg = config();
		cg.writeEntry( "ProviderURL", provider_url );
		emit configNeedsSaving();

		paint_helper->setValues( humidity, wind, invertWindRose, windRoseScale, rain, dewPoint, visibility,
			pressure, uvIndex, tempType, icons, iconsCustom, background, backgroundColor,
			backgroundTransparency, scaleIcons, font, fontColor, fontShadow, scaleFont,
			forecastSeparator, layoutDirection(), data_custom_image_list.size() > 0 ? data_custom_image_list.at( 0 ) : QByteArray(),
			data_current_wind, data_current_humidity, data_current_rain, data_current_dew_point,
			data_current_visibility, data_current_pressure, data_current_uv_index );
	}
	custom_image_dialog->setFont( font );

	double font_scale = getFontScale();

	if ( data_update_time == "" )
	{
		kDebug() << "Not showing anything, as one of the downloads failed (and no data is in cache).";
		kDebug() << "Did you specify an invalid address? Do you have a working internet connection?";

		setBusy( true );

		if ( size().height() < 150. )
		{
			Plasma::Svg* svg;
			svg = new Plasma::Svg( this );
			svg->setImagePath( "widgets/busywidget" );
			svg->setContainsMultipleImages( false );

			QSize size;
			size = svg->pixmap().size();
			size.scale( 0.5 * contents_rect.size(), Qt::KeepAspectRatio );

			QRect scaled_rect = QRect( contents_rect.x() + (int)( 0.5 * ( contents_rect.width() - size.width() ) ), contents_rect.y() + (int)( 0.5 * ( contents_rect.height() - size.height() ) ), size.width(), size.height() );

			svg->resize( scaled_rect.size() );

			svg->paint( p, scaled_rect );

			p->end();
		}

		delete p;
		p = NULL;

		updating = false;

		return;
	}

	setBusy( false );

	int x_0 = contents_rect.left();
	int y_0 = contents_rect.top();
	int width = contents_rect.width();
	int height = contents_rect.height();

	QRect title_rect;
	QRect clock_rect;

	QRect current_complete_rect;
	QRect current_temp_rect;
	QRect current_icon_rect;
	QRect current_icon_text_rect;
	QRect current_wind_rect;
	QRect current_sun_rect;

	QRect custom_image_rect( 0, 0, 0, 0 );

	QRect day_complete_rect[7];
	QRect day_name_rect[7];
	QRect day_temp_rect[7];
	QRect day_icon_rect[7];
	QRect day_icon_text_rect[7];

	p->setRenderHints( QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing | QPainter::Antialiasing );
	p->setPen( fontColor );

	QFont update_time_font( font, (int)(scaleFont*font_scale*9), QFont::Normal );
	QFont provider_update_time_font( font, (int)(scaleFont*font_scale*9), QFont::Normal, true );
	QFont title_font( font, (int)(scaleFont*font_scale*11), QFont::Bold );
	QFont current_temperature_font( font, (int)(scaleFont*font_scale*24), QFont::Normal );
	QFont current_temperature_felt_font( font, (int)(scaleFont*font_scale*9), QFont::Normal );
	QFont current_wind_sun_font( font, (int)(scaleFont*font_scale*9), QFont::Normal );
	QFont current_wind_nsew_font( font, (int)(scaleFont*font_scale*7.5), QFont::Bold );
	QFont current_icon_text_font( font, (int)(scaleFont*font_scale*9), QFont::Normal );

	QFont day_name_font( font, (int)(scaleFont*font_scale*11), QFont::Bold );
	QFont day_temperature_font( font, (int)(scaleFont*font_scale*11), QFont::Normal );
	QFont day_icon_text_font( font, (int)(scaleFont*font_scale*9), QFont::Normal );

	int height_title;
	int days = 0;

	double space_percentage = 0.05;

	if ( forecastSeparator == 2 ) space_percentage = 0.0;

	const double custom_image_link_scale = 0.85;

	switch( layoutNumber )
	{
		case DAYS_1_HORIZONTAL: // 1 horizontal
			height_title = (int)(((double)(height)/(1+space_percentage))/5);
			clock_rect = QRect( x_0, y_0, width, height_title );
			custom_image_rect = QRect( x_0, y_0, (int)(custom_image_link_scale*height_title), (int)(custom_image_link_scale*height_title) );
			title_rect = QRect(x_0, y_0, width, height_title);
			y_0 = y_0 + height_title;
			height = height - height_title;
			paint_helper->getRectCurrentHoriz( x_0, y_0, width, height, 4, 1, 0, 0, current_complete_rect,
								 current_temp_rect, current_icon_rect, current_icon_text_rect, current_wind_rect, current_sun_rect );
			paint_helper->getRectForecast( x_0, y_0, width, height, 4, 1, 3, 0,
							 day_complete_rect[0], day_name_rect[0], day_temp_rect[0], day_icon_rect[0], day_icon_text_rect[0] );

			days = 1;
			break;
		case DAYS_1_VERTICAL: // 1 vertical
			height_title = (int)(((double)(height)/(2+space_percentage))/5);
			clock_rect = QRect( x_0, y_0, width, height_title );
			custom_image_rect = QRect( x_0, y_0, (int)(custom_image_link_scale*height_title), (int)(custom_image_link_scale*height_title) );
			title_rect = QRect(x_0, y_0 + (int)(0.5*height_title), width, height_title );
			y_0 = y_0 + height_title + (int)(0.5*height_title);
			height = height - height_title - (int)(0.5*height_title);
			paint_helper->getRectCurrentVert( x_0, y_0, width, height, 2, 2+space_percentage, 0, 0, current_complete_rect,
								current_temp_rect, current_icon_rect, current_icon_text_rect, current_wind_rect, current_sun_rect );

			paint_helper->getRectForecast( x_0, y_0, width, height, 2, 2+space_percentage, 1, 1+space_percentage,
							 day_complete_rect[0], day_name_rect[0], day_temp_rect[0], day_icon_rect[0], day_icon_text_rect[0] );
			days = 1;
			break;
		case DAYS_3_HORIZONTAL: // 3 horizontal
			height_title = (int)(((double)(height)/(2+space_percentage))/5);
			clock_rect = QRect( x_0, y_0, width, height_title );
			custom_image_rect = QRect( x_0, y_0, (int)(custom_image_link_scale*height_title), (int)(custom_image_link_scale*height_title) );
			title_rect = QRect(x_0, y_0, width, height_title);
			y_0 = y_0 + height_title;
			height = height - height_title;
			paint_helper->getRectCurrentHoriz( x_0, y_0, width, height, 3, 2+space_percentage, 0, 0, current_complete_rect,
								 current_temp_rect, current_icon_rect, current_icon_text_rect, current_wind_rect, current_sun_rect );
			paint_helper->getRectForecast( x_0, y_0, width, height, 3, 2+space_percentage, 0, 1+space_percentage,
							 day_complete_rect[0], day_name_rect[0], day_temp_rect[0], day_icon_rect[0], day_icon_text_rect[0] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 3, 2+space_percentage, 1, 1+space_percentage,
							 day_complete_rect[1], day_name_rect[1], day_temp_rect[1], day_icon_rect[1], day_icon_text_rect[1] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 3, 2+space_percentage, 2, 1+space_percentage,
							 day_complete_rect[2], day_name_rect[2], day_temp_rect[2], day_icon_rect[2], day_icon_text_rect[2] );

			days = 3;
			break;
		case DAYS_3_VERTICAL: // 3 vertical
			height_title = (int)(((double)(height)/(3+2*space_percentage))/5);
			clock_rect = QRect( x_0, y_0, width, height_title );
			custom_image_rect = QRect( x_0, y_0, (int)(custom_image_link_scale*height_title), (int)(custom_image_link_scale*height_title) );
			title_rect = QRect(x_0, y_0 + (int)(0.5*height_title), width, height_title );
			y_0 = y_0 + height_title + (int)(0.5*height_title);
			height = height - height_title - (int)(0.5*height_title);
			paint_helper->getRectCurrentVert( x_0, y_0, width, height, 2, 3+2*space_percentage, 0, 0, current_complete_rect,
								current_temp_rect, current_icon_rect, current_icon_text_rect, current_wind_rect, current_sun_rect );

			paint_helper->getRectForecast( x_0, y_0, width, height, 2, 3+2*space_percentage, 1, 1+space_percentage,
							 day_complete_rect[0], day_name_rect[0], day_temp_rect[0], day_icon_rect[0], day_icon_text_rect[0] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 2, 3+2*space_percentage, 0, 2+2*space_percentage,
							 day_complete_rect[1], day_name_rect[1], day_temp_rect[1], day_icon_rect[1], day_icon_text_rect[1] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 2, 3+2*space_percentage, 1, 2+2*space_percentage,
							 day_complete_rect[2], day_name_rect[2], day_temp_rect[2], day_icon_rect[2], day_icon_text_rect[2] );
			days = 3;
			break;
		case DAYS_5_HORIZONTAL: // 5 horizontal
			height_title = (int)(((double)(height)/(2+space_percentage))/5);
			clock_rect = QRect( x_0, y_0, width, height_title );
			custom_image_rect = QRect( x_0, y_0, (int)(custom_image_link_scale*height_title), (int)(custom_image_link_scale*height_title) );
			title_rect = QRect( x_0, y_0, width, height_title );
			y_0 = y_0 + height_title;
			height = height - height_title;
			paint_helper->getRectCurrentHoriz( x_0, y_0, width, height, 5, 2+space_percentage, 1, 0, current_complete_rect,
								 current_temp_rect, current_icon_rect, current_icon_text_rect, current_wind_rect, current_sun_rect );
			paint_helper->getRectForecast( x_0, y_0, width, height, 5, 2+space_percentage, 0, 1+space_percentage,
							 day_complete_rect[0], day_name_rect[0], day_temp_rect[0], day_icon_rect[0], day_icon_text_rect[0] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 5, 2+space_percentage, 1, 1+space_percentage,
							 day_complete_rect[1], day_name_rect[1], day_temp_rect[1], day_icon_rect[1], day_icon_text_rect[1] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 5, 2+space_percentage, 2, 1+space_percentage,
							 day_complete_rect[2], day_name_rect[2], day_temp_rect[2], day_icon_rect[2], day_icon_text_rect[2] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 5, 2+space_percentage, 3, 1+space_percentage,
							 day_complete_rect[3], day_name_rect[3], day_temp_rect[3], day_icon_rect[3], day_icon_text_rect[3] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 5, 2+space_percentage, 4, 1+space_percentage,
							 day_complete_rect[4], day_name_rect[4], day_temp_rect[4], day_icon_rect[4], day_icon_text_rect[4] );
			days = 5;
			break;
		case DAYS_5_HORIZONTAL_COMPACT: // 5 horizontal (compact)
			height_title = (int)(((double)(height)/(2+space_percentage))/5);
			clock_rect = QRect( x_0, y_0, width, height_title );
			custom_image_rect = QRect( x_0, y_0, (int)(custom_image_link_scale*height_title), (int)(custom_image_link_scale*height_title) );
			title_rect = QRect( x_0, y_0, width, height_title );
			y_0 = y_0 + height_title;
			height = height - height_title;
			paint_helper->getRectCurrentHoriz( x_0, y_0, width, height, 4, 2+space_percentage, 0, 0, current_complete_rect,
								 current_temp_rect, current_icon_rect, current_icon_text_rect, current_wind_rect, current_sun_rect );
			paint_helper->getRectForecast( x_0, y_0, width, height, 4, 2+space_percentage, 3, 0,
							 day_complete_rect[0], day_name_rect[0], day_temp_rect[0], day_icon_rect[0], day_icon_text_rect[0] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 4, 2+space_percentage, 0, 1+space_percentage,
							 day_complete_rect[1], day_name_rect[1], day_temp_rect[1], day_icon_rect[1], day_icon_text_rect[1] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 4, 2+space_percentage, 1, 1+space_percentage,
							 day_complete_rect[2], day_name_rect[2], day_temp_rect[2], day_icon_rect[2], day_icon_text_rect[2] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 4, 2+space_percentage, 2, 1+space_percentage,
							 day_complete_rect[3], day_name_rect[3], day_temp_rect[3], day_icon_rect[3], day_icon_text_rect[3] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 4, 2+space_percentage, 3, 1+space_percentage,
							 day_complete_rect[4], day_name_rect[4], day_temp_rect[4], day_icon_rect[4], day_icon_text_rect[4] );
			current_complete_rect = QRect( 0, 0, 0, 0 ); // disable separators for now
			days = 5;
			break;
		case DAYS_5_VERTICAL: // 5 vertical
			height_title = (int)(((double)(height)/(4+3*space_percentage))/5);
			clock_rect = QRect( x_0, y_0, width, height_title );
			custom_image_rect = QRect( x_0, y_0, (int)(custom_image_link_scale*height_title), (int)(custom_image_link_scale*height_title) );
			title_rect = QRect(x_0, y_0 + (int)(0.5*height_title), width, height_title );
			y_0 = y_0 + height_title + (int)(0.5*height_title);
			height = height - height_title - (int)(0.5*height_title);
			paint_helper->getRectCurrentVert( x_0, y_0, width, height, 2, 4+3*space_percentage, 0, 0,
								current_complete_rect, current_temp_rect, current_icon_rect, current_icon_text_rect, current_wind_rect, current_sun_rect );

			paint_helper->getRectForecast( x_0, y_0, width, height, 2, 4+3*space_percentage, 1, 1+space_percentage,
							 day_complete_rect[0], day_name_rect[0], day_temp_rect[0], day_icon_rect[0], day_icon_text_rect[0] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 2, 4+3*space_percentage, 0, 2+2*space_percentage,
							 day_complete_rect[1], day_name_rect[1], day_temp_rect[1], day_icon_rect[1], day_icon_text_rect[1] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 2, 4+3*space_percentage, 1, 2+2*space_percentage,
							 day_complete_rect[2], day_name_rect[2], day_temp_rect[2], day_icon_rect[2], day_icon_text_rect[2] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 2, 4+3*space_percentage, 0, 3+3*space_percentage,
							 day_complete_rect[3], day_name_rect[3], day_temp_rect[3], day_icon_rect[3], day_icon_text_rect[3] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 2, 4+3*space_percentage, 1, 3+3*space_percentage,
							 day_complete_rect[4], day_name_rect[4], day_temp_rect[4], day_icon_rect[4], day_icon_text_rect[4] );
			days = 5;
			break;
		case DAYS_7_HORIZONTAL: // 7 horizontal
			height_title = (int)(((double)(height)/(2+space_percentage))/5);
			clock_rect = QRect( x_0, y_0, width, height_title );
			custom_image_rect = QRect( x_0, y_0, (int)(custom_image_link_scale*height_title), (int)(custom_image_link_scale*height_title) );
			title_rect = QRect(x_0, y_0, width, height_title );
			y_0 = y_0 + height_title;
			height = height - height_title;
			paint_helper->getRectCurrentHoriz( x_0, y_0, width, height, 7, 2+space_percentage, 2, 0,
								 current_complete_rect, current_temp_rect, current_icon_rect, current_icon_text_rect, current_wind_rect, current_sun_rect );
			paint_helper->getRectForecast( x_0, y_0, width, height, 7, 2+space_percentage, 0, 1+space_percentage,
							 day_complete_rect[0], day_name_rect[0], day_temp_rect[0], day_icon_rect[0], day_icon_text_rect[0] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 7, 2+space_percentage, 1, 1+space_percentage,
							 day_complete_rect[1], day_name_rect[1], day_temp_rect[1], day_icon_rect[1], day_icon_text_rect[1] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 7, 2+space_percentage, 2, 1+space_percentage,
							 day_complete_rect[2], day_name_rect[2], day_temp_rect[2], day_icon_rect[2], day_icon_text_rect[2] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 7, 2+space_percentage, 3, 1+space_percentage,
							 day_complete_rect[3], day_name_rect[3], day_temp_rect[3], day_icon_rect[3], day_icon_text_rect[3] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 7, 2+space_percentage, 4, 1+space_percentage,
							 day_complete_rect[4], day_name_rect[4], day_temp_rect[4], day_icon_rect[4], day_icon_text_rect[4] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 7, 2+space_percentage, 5, 1+space_percentage,
							 day_complete_rect[5], day_name_rect[5], day_temp_rect[5], day_icon_rect[5], day_icon_text_rect[5] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 7, 2+space_percentage, 6, 1+space_percentage,
							 day_complete_rect[6], day_name_rect[6], day_temp_rect[6], day_icon_rect[6], day_icon_text_rect[6] );
			days = 7;
			break;
		case DAYS_7_HORIZONTAL_COMPACT: // 7 horizontal (compact)
			height_title = (int)(((double)(height)/(2+space_percentage))/5);
			clock_rect = QRect( x_0, y_0, width, height_title );
			custom_image_rect = QRect( x_0, y_0, (int)(custom_image_link_scale*height_title), (int)(custom_image_link_scale*height_title) );
			title_rect = QRect(x_0, y_0, width, height_title );
			y_0 = y_0 + height_title;
			height = height - height_title;
			paint_helper->getRectCurrentHoriz( x_0, y_0, width, height, 5, 2+space_percentage, 0, 0,
								 current_complete_rect, current_temp_rect, current_icon_rect, current_icon_text_rect, current_wind_rect, current_sun_rect );
			paint_helper->getRectForecast( x_0, y_0, width, height, 5, 2+space_percentage, 3, 0,
							 day_complete_rect[0], day_name_rect[0], day_temp_rect[0], day_icon_rect[0], day_icon_text_rect[0] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 5, 2+space_percentage, 4, 0,
							 day_complete_rect[1], day_name_rect[1], day_temp_rect[1], day_icon_rect[1], day_icon_text_rect[1] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 5, 2+space_percentage, 0, 1+space_percentage,
							 day_complete_rect[2], day_name_rect[2], day_temp_rect[2], day_icon_rect[2], day_icon_text_rect[2] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 5, 2+space_percentage, 1, 1+space_percentage,
							 day_complete_rect[3], day_name_rect[3], day_temp_rect[3], day_icon_rect[3], day_icon_text_rect[3] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 5, 2+space_percentage, 2, 1+space_percentage,
							 day_complete_rect[4], day_name_rect[4], day_temp_rect[4], day_icon_rect[4], day_icon_text_rect[4] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 5, 2+space_percentage, 3, 1+space_percentage,
							 day_complete_rect[5], day_name_rect[5], day_temp_rect[5], day_icon_rect[5], day_icon_text_rect[5] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 5, 2+space_percentage, 4, 1+space_percentage,
							 day_complete_rect[6], day_name_rect[6], day_temp_rect[6], day_icon_rect[6], day_icon_text_rect[6] );
			current_complete_rect = QRect( 0, 0, 0, 0 ); // disable separators for now
			days = 7;
			break;
		case DAYS_7_VERTICAL: // 7 vertical
			height_title = (int)(((double)(height)/(5+4*space_percentage))/5);
			clock_rect = QRect( x_0, y_0, width, height_title );
			custom_image_rect = QRect( x_0, y_0, (int)(custom_image_link_scale*height_title), (int)(custom_image_link_scale*height_title) );
			title_rect = QRect(x_0, y_0 + (int)(0.5*height_title), width, height_title );
			y_0 = y_0 + height_title + (int)(0.5*height_title);
			height = height - height_title - (int)(0.5*height_title);
			paint_helper->getRectCurrentVert( x_0, y_0, width, height, 2, 5+4*space_percentage, 0, 0,
								current_complete_rect, current_temp_rect, current_icon_rect, current_icon_text_rect, current_wind_rect, current_sun_rect );

			paint_helper->getRectForecast( x_0, y_0, width, height, 2, 5+4*space_percentage, 1, 1+space_percentage,
							 day_complete_rect[0], day_name_rect[0], day_temp_rect[0], day_icon_rect[0], day_icon_text_rect[0] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 2, 5+4*space_percentage, 0, 2+2*space_percentage,
							 day_complete_rect[1], day_name_rect[1], day_temp_rect[1], day_icon_rect[1], day_icon_text_rect[1] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 2, 5+4*space_percentage, 1, 2+2*space_percentage,
							 day_complete_rect[2], day_name_rect[2], day_temp_rect[2], day_icon_rect[2], day_icon_text_rect[2] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 2, 5+4*space_percentage, 0, 3+3*space_percentage,
							 day_complete_rect[3], day_name_rect[3], day_temp_rect[3], day_icon_rect[3], day_icon_text_rect[3] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 2, 5+4*space_percentage, 1, 3+3*space_percentage,
							 day_complete_rect[4], day_name_rect[4], day_temp_rect[4], day_icon_rect[4], day_icon_text_rect[4] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 2, 5+4*space_percentage, 0, 4+4*space_percentage,
							 day_complete_rect[5], day_name_rect[5], day_temp_rect[5], day_icon_rect[5], day_icon_text_rect[5] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 2, 5+4*space_percentage, 1, 4+4*space_percentage,
							 day_complete_rect[6], day_name_rect[6], day_temp_rect[6], day_icon_rect[6], day_icon_text_rect[6] );
			days = 7;
			break;
		case DAYS_4_CUSTOM_IMAGE_HORIZONTAL:
			height_title = (int)(((double)(height)/(2+space_percentage))/5);
			clock_rect = QRect( x_0, y_0, width, height_title );
			title_rect = QRect(x_0, y_0, width, height_title );
			y_0 = y_0 + height_title;
			height = height - height_title;
			paint_helper->getRectCurrentHoriz( x_0, y_0, width, height, 4, 2+space_percentage, 0, 0,
								 current_complete_rect, current_temp_rect, current_icon_rect, current_icon_text_rect, current_wind_rect, current_sun_rect );
			paint_helper->getRectForecast( x_0, y_0, width, height, 4, 2+space_percentage, 3, 0,
							 day_complete_rect[0], day_name_rect[0], day_temp_rect[0], day_icon_rect[0], day_icon_text_rect[0] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 4, 2+space_percentage, 1, 1+space_percentage,
							 day_complete_rect[1], day_name_rect[1], day_temp_rect[1], day_icon_rect[1], day_icon_text_rect[1] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 4, 2+space_percentage, 2, 1+space_percentage,
							 day_complete_rect[2], day_name_rect[2], day_temp_rect[2], day_icon_rect[2], day_icon_text_rect[2] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 4, 2+space_percentage, 3, 1+space_percentage,
							 day_complete_rect[3], day_name_rect[3], day_temp_rect[3], day_icon_rect[3], day_icon_text_rect[3] );
			current_complete_rect = QRect( 0, 0, 0, 0 ); // disable separators for now
			custom_image_rect = QRect( x_0, y_0 + (int)( ( 1 + space_percentage ) * height / ( 2 + space_percentage ) ), (int)( width / 4 ), (int)( height / ( 2 + space_percentage ) ) );
			days = 4;
			break;
		case DAYS_4_CUSTOM_IMAGE_VERTICAL:
			paint_helper->getRectCurrentLoc( x_0, y_0, width, height, 3.5, 3+2*space_percentage, 0, 0,
							   current_complete_rect, current_temp_rect, current_wind_rect, current_sun_rect );
			paint_helper->getRectLocationImage( x_0, y_0, width, height, 3.5, 3+2*space_percentage, 0, 0,
								  custom_image_rect, current_icon_rect, current_icon_text_rect, title_rect, clock_rect );
			paint_helper->getRectForecast( x_0, y_0, width, height, 3.5, 3+2*space_percentage, 1.5, 1+space_percentage,
							 day_complete_rect[0], day_name_rect[0], day_temp_rect[0], day_icon_rect[0], day_icon_text_rect[0] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 3.5, 3+2*space_percentage, 2.5, 1+space_percentage,
							 day_complete_rect[1], day_name_rect[1], day_temp_rect[1], day_icon_rect[1], day_icon_text_rect[1] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 3.5, 3+2*space_percentage, 1.5, 2+2*space_percentage,
							 day_complete_rect[2], day_name_rect[2], day_temp_rect[2], day_icon_rect[2], day_icon_text_rect[2] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 3.5, 3+2*space_percentage, 2.5, 2+2*space_percentage,
							 day_complete_rect[3], day_name_rect[3], day_temp_rect[3], day_icon_rect[3], day_icon_text_rect[3] );
			days = 4;
			break;
		case DAYS_7_CUSTOM_IMAGE: // 7 location image
			paint_helper->getRectCurrentLoc( x_0, y_0, width, height, 4.5, 3+2*space_percentage, 0, 0,
							   current_complete_rect, current_temp_rect, current_wind_rect, current_sun_rect );
			paint_helper->getRectLocationImage( x_0, y_0, width, height, 4.5, 3+2*space_percentage, 0, 0,
								  custom_image_rect, current_icon_rect, current_icon_text_rect, title_rect, clock_rect );
			paint_helper->getRectForecast( x_0, y_0, width, height, 4.5, 3+2*space_percentage, 3.5, 0,
							 day_complete_rect[0], day_name_rect[0], day_temp_rect[0], day_icon_rect[0], day_icon_text_rect[0] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 4.5, 3+2*space_percentage, 1.5, 1+space_percentage,
							 day_complete_rect[1], day_name_rect[1], day_temp_rect[1], day_icon_rect[1], day_icon_text_rect[1] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 4.5, 3+2*space_percentage, 2.5, 1+space_percentage,
							 day_complete_rect[2], day_name_rect[2], day_temp_rect[2], day_icon_rect[2], day_icon_text_rect[2] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 4.5, 3+2*space_percentage, 3.5, 1+space_percentage,
							 day_complete_rect[3], day_name_rect[3], day_temp_rect[3], day_icon_rect[3], day_icon_text_rect[3] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 4.5, 3+2*space_percentage, 1.5, 2+2*space_percentage,
							 day_complete_rect[4], day_name_rect[4], day_temp_rect[4], day_icon_rect[4], day_icon_text_rect[4] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 4.5, 3+2*space_percentage, 2.5, 2+2*space_percentage,
							 day_complete_rect[5], day_name_rect[5], day_temp_rect[5], day_icon_rect[5], day_icon_text_rect[5] );
			paint_helper->getRectForecast( x_0, y_0, width, height, 4.5, 3+2*space_percentage, 3.5, 2+2*space_percentage,
							 day_complete_rect[6], day_name_rect[6], day_temp_rect[6], day_icon_rect[6], day_icon_text_rect[6] );
			days = 7;
			break;
		default:
			break;
	}

	if ( data_custom_image_list.size() > 0 && customImageList.size() > 0
		&& ( customImageList.size() > 1 || !data_custom_image_list.at( 0 ).isEmpty() ) )
	{
		setLocationImageRect( custom_image_rect );
	}
	else
	{
		setLocationImageRect( QRect( 0, 0, 0, 0 ) );
	}

	setCurrentWeatherRect( current_temp_rect, current_icon_rect, current_icon_text_rect, current_wind_rect, current_sun_rect );

	if ( layoutNumber == DAYS_1_HORIZONTAL || layoutNumber == DAYS_1_VERTICAL || layoutNumber == DAYS_3_HORIZONTAL || layoutNumber == DAYS_3_VERTICAL ||
		layoutNumber == DAYS_5_HORIZONTAL || layoutNumber == DAYS_5_HORIZONTAL_COMPACT || layoutNumber == DAYS_5_VERTICAL ||
		layoutNumber == DAYS_7_HORIZONTAL || layoutNumber == DAYS_7_HORIZONTAL_COMPACT || layoutNumber == DAYS_7_VERTICAL )
	{
		if ( data_custom_image_list.size() > 0 && customImageList.size() > 0 )
		{
			if ( customImageList.size() > 1 || !data_custom_image_list.at( 0 ).isEmpty() )
			{
				Plasma::Svg* svg;
				svg = new Plasma::Svg( this );
				svg->setImagePath( "widgets/satellite_image" );
				svg->setContainsMultipleImages( false );

				svg->resize( custom_image_rect.size() );

				p->drawPixmap( custom_image_rect, svg->pixmap() );

				delete svg;
				svg = NULL;
			}
		}
	}
	else
	{
		if ( customImageList.size() > 1 || ( data_custom_image_list.size() >= 1 && !data_custom_image_list.at( 0 ).isEmpty() ) )
		{
			paint_helper->drawLocationImage( p, custom_image_rect );
		}
	}

	if ( provider_update_time_shown )
	{
		p->setFont( provider_update_time_font );
		setUpdateTimeRect( p->boundingRect( clock_rect,
			Qt::AlignTop | ( ( layoutNumber == DAYS_4_CUSTOM_IMAGE_VERTICAL || layoutNumber == DAYS_7_CUSTOM_IMAGE ) ?
			Qt::AlignLeft : Qt::AlignRight ), data_provider_update_time ) );
	}
	else
	{
		p->setFont( update_time_font );
		setUpdateTimeRect( p->boundingRect( clock_rect,
			Qt::AlignTop | ( ( layoutNumber == DAYS_4_CUSTOM_IMAGE_VERTICAL || layoutNumber == DAYS_7_CUSTOM_IMAGE ) ?
			Qt::AlignLeft : Qt::AlignRight ), data_update_time ) );
	}

	int i;
	if ( omitIconDescription ) // resize icon rect to icon_rect + icon_text_rect
	{
		current_icon_rect = current_icon_rect.united( current_icon_text_rect );
		for ( i=0; i<7; i++ ) day_icon_rect[i] = day_icon_rect[i].united( day_icon_text_rect[i] );
	}

	// Draw icons before anything else to have text above icons, if necessary
	if ( icons > 0 )
	{
		if ( backgroundFile < background_file_list.size() && backgroundFile >= 0 && background_file_list.size() > 0 )
			paint_helper->paintWeatherSVG( p, current_icon_rect, data_current_icon_code, background_file_list.at(backgroundFile).file_name, true );
	}
	else
	{
		if ( backgroundFile < background_file_list.size() && backgroundFile >= 0 && background_file_list.size() > 0 )
			paint_helper->paintWeatherIcon( p, current_icon_rect, data_current_icon, background_file_list.at(backgroundFile).file_name, true );
	}

	// location + country
	QString location_country = "";
	if( data_location_location != "" ) location_country = location_country + data_location_location;
	if( data_location_location != "" && data_location_country != "" )
	{
		if ( layoutNumber == DAYS_7_CUSTOM_IMAGE || layoutNumber == DAYS_4_CUSTOM_IMAGE_VERTICAL )
		{
			location_country = location_country + "\n";
		}
		else
		{
			location_country = location_country + ", ";
		}
	}
	if( data_location_country != "" ) location_country = location_country + data_location_country;
	p->setFont( paint_helper->getFittingFontSize( p, title_rect, Qt::AlignVCenter | Qt::AlignHCenter,
		location_country, title_font, true, true, 0.3, 0, 0 ) );
	paint_helper->drawTextWithShadow( p, title_rect, Qt::AlignVCenter | Qt::AlignHCenter, location_country );

	if ( updateTime )
	{
		int time_width = 0;
		int time_height = 0;

		if ( provider_update_time_shown )
		{
			p->setFont( paint_helper->getFittingFontSize( p, clock_rect, Qt::AlignTop |
				( ( layoutNumber == DAYS_4_CUSTOM_IMAGE_VERTICAL || layoutNumber == DAYS_7_CUSTOM_IMAGE ) ? Qt::AlignLeft : Qt::AlignRight ),
				data_provider_update_time, provider_update_time_font, true, true, 0.3, 0, 0 ) );
			QRect time_bb = p->boundingRect( clock_rect, Qt::AlignTop | ( ( layoutNumber == DAYS_4_CUSTOM_IMAGE_VERTICAL || layoutNumber == DAYS_7_CUSTOM_IMAGE ) ?
				Qt::AlignLeft : Qt::AlignRight ), data_provider_update_time );
			time_width = time_bb.width();
			time_height = time_bb.height();
			paint_helper->drawTextWithShadow( p, clock_rect,
				Qt::AlignTop | ( ( layoutNumber == DAYS_4_CUSTOM_IMAGE_VERTICAL || layoutNumber == DAYS_7_CUSTOM_IMAGE ) ?
				Qt::AlignLeft : Qt::AlignRight ), data_provider_update_time );
		}
		else
		{
			p->setFont( paint_helper->getFittingFontSize( p, clock_rect, Qt::AlignTop |
				( ( layoutNumber == DAYS_4_CUSTOM_IMAGE_VERTICAL || layoutNumber == DAYS_7_CUSTOM_IMAGE ) ? Qt::AlignLeft : Qt::AlignRight ),
				data_update_time, update_time_font, true, true, 0.3, 0, 0 ) );
			QRect time_bb = p->boundingRect( clock_rect, Qt::AlignTop | ( ( layoutNumber == DAYS_4_CUSTOM_IMAGE_VERTICAL || layoutNumber == DAYS_7_CUSTOM_IMAGE ) ?
				Qt::AlignLeft : Qt::AlignRight ), data_update_time );
			time_width = time_bb.width();
			time_height = time_bb.height();
			paint_helper->drawTextWithShadow( p, clock_rect,
				Qt::AlignTop | ( ( layoutNumber == DAYS_4_CUSTOM_IMAGE_VERTICAL || layoutNumber == DAYS_7_CUSTOM_IMAGE ) ?
				Qt::AlignLeft : Qt::AlignRight ), data_update_time );
		}

		if ( !last_download_succeeded )
		{
			int icon_size = time_height;
			QPixmap disconnected_icon = KIconLoader::global()->loadIcon( "network-disconnect", KIconLoader::NoGroup, icon_size, KIconLoader::DefaultState, QStringList(), 0L, false );

			if ( layoutNumber == DAYS_4_CUSTOM_IMAGE_VERTICAL || layoutNumber == DAYS_7_CUSTOM_IMAGE ) // left align
			{
				p->drawPixmap( QRect( clock_rect.left() + time_width, clock_rect.top(), icon_size, icon_size ), disconnected_icon );
			}
			else // right align
			{
				p->drawPixmap( QRect( clock_rect.right() - time_width - icon_size, clock_rect.top(), icon_size, icon_size ), disconnected_icon );
			}
		}
	}

	// draw current values
	// background
	if ( forecastSeparator == 1 )
	{
		paint_helper->drawGradient( p, current_complete_rect );
	}
	else if ( forecastSeparator == 2 )
	{
		paint_helper->drawPlasmaBackground( p, current_complete_rect, ( background == 1 || background == 2 || background == 3 )/*translucent*/ );
	}

	// temperature
	int feels_like_height = 0;
	if( data_current_temperature_felt != "" && data_current_temperature_felt.contains(QRegExp("[0-9]")) )
	{
		// maximum height: 0.5 * space
		QString temperature_felt_string = QString( feelsLike != "" ? feelsLike + "\n" : "" ) + data_current_temperature_felt + QChar(0xB0) + ( showTemperatureUnit ? tempType : "" );
		p->setFont( paint_helper->getFittingFontSize( p, QRect( current_temp_rect.x(), current_temp_rect.y() + (int)( 0.5 * current_temp_rect.height() ),
			current_temp_rect.width(), (int)( 0.5 * current_temp_rect.height() ) ),
			Qt::AlignBottom | Qt::AlignHCenter, temperature_felt_string,
			current_temperature_felt_font, true, true, 0.3, 0, &feels_like_height ) );
		paint_helper->drawTextWithShadow( p, current_temp_rect, Qt::AlignBottom | Qt::AlignHCenter, temperature_felt_string );
	}

	QRect current_temp_remaining_rect( current_temp_rect.x(), current_temp_rect.y(), current_temp_rect.width(), current_temp_rect.height() - feels_like_height );

	int temperature_height = 0;
	p->setFont( paint_helper->getFittingFontSize( p, current_temp_remaining_rect, Qt::AlignTop | Qt::AlignHCenter,
		data_current_temperature + QChar(0xB0) + ( showTemperatureUnit ? tempType : "" ), current_temperature_font, true, true, 0.3, 0, &temperature_height ) );

	if ( temperature_height > 0 )
	{
		current_temp_remaining_rect = QRect( current_temp_remaining_rect.x(),
			current_temp_remaining_rect.y() + (int)( floor( 0.5 * ( current_temp_remaining_rect.height() - temperature_height ) ) ),
			current_temp_remaining_rect.width(),
			(int)( current_temp_remaining_rect.height() - floor( 0.5 * ( current_temp_remaining_rect.height() - temperature_height ) ) ) );
	}

	paint_helper->drawTextWithShadow( p, current_temp_remaining_rect, Qt::AlignTop | Qt::AlignHCenter,
		data_current_temperature + QChar(0xB0) + ( showTemperatureUnit ? tempType : "" ) );

	// icon text
	if ( !omitIconDescription )
	{
		p->setFont( paint_helper->getFittingFontSize( p, current_icon_text_rect, Qt::AlignBottom | Qt::AlignHCenter | Qt::TextWordWrap,
			data_current_icon_text, current_icon_text_font, false, true, 0.2, 0, 0 ) );
		paint_helper->drawTextWithShadow( p, current_icon_text_rect, Qt::AlignBottom | Qt::AlignHCenter | Qt::TextWordWrap, data_current_icon_text );
	}

	// sunrise / sunset
	QString sunrise_sunset = "";
	if ( data_sun_sunrise != "" && data_sun_sunrise.contains(QRegExp("[0-9]")) ) sunrise_sunset = sunrise_sunset + data_sun_sunrise;
	if ( data_sun_sunrise != "" && data_sun_sunset != "" && data_sun_sunrise.contains(QRegExp("[0-9]")) &&
		data_sun_sunset.contains(QRegExp("[0-9]")) ) sunrise_sunset = sunrise_sunset + " / ";
	if ( data_sun_sunset != "" && data_sun_sunset.contains(QRegExp("[0-9]")) ) sunrise_sunset = sunrise_sunset + data_sun_sunset;

	if ( sunrise_sunset != "" )
	{
		p->setFont(paint_helper->getFittingFontSize( p, current_sun_rect, Qt::AlignBottom | Qt::AlignHCenter,
			sunrise_sunset, current_wind_sun_font, true, true, 0.3, 0, 0 ) );

		paint_helper->drawTextWithShadow( p, current_sun_rect, Qt::AlignBottom | Qt::AlignHCenter, sunrise_sunset );
	}
	else
	{
		// No sunrise/sunset? --> Wind rose may eat up sunrise/sunset space
		current_wind_rect = current_wind_rect.united( current_sun_rect );
	}

	// wind rose
	p->setFont( current_wind_sun_font );
	if ( !windLayoutRose )
	{
		QString wind_humidity = "";
		if ( wind != "" ) wind_humidity = wind_humidity + wind + ": ";
		if ( data_current_wind != "" ) wind_humidity = wind_humidity + data_current_wind;

		p->setFont( paint_helper->getFittingFontSize( p, current_wind_rect, Qt::AlignVCenter | Qt::AlignHCenter | Qt::TextWordWrap,
			wind_humidity, current_wind_sun_font, false, true, 0.2, 0, 0 ) );

		paint_helper->drawTextWithShadow( p, current_wind_rect, Qt::AlignVCenter | Qt::AlignHCenter | Qt::TextWordWrap, wind_humidity );
	}
	else
	{
		paint_helper->drawWind( p, current_wind_rect, data_current_wind_code, data_current_wind_speed, wind, windRose, current_wind_sun_font, current_wind_nsew_font );
	}

	// draw forecast values
	for( i=0; i<days; i++ )
	{
		// separators
		if ( forecastSeparator == 1 ) paint_helper->drawGradient( p, day_complete_rect[i] );
		else if ( forecastSeparator == 2 ) paint_helper->drawPlasmaBackground( p, day_complete_rect[i], ( background == 1 || background == 2 || background == 3 )/*translucent*/ );

		// icons
		if ( icons > 0 )
		{
			if ( backgroundFile < background_file_list.size() && backgroundFile >= 0 && background_file_list.size() > 0 )
				paint_helper->paintWeatherSVG( p,	day_icon_rect[i], data_day_icon_code[i], background_file_list.at(backgroundFile).file_name, true );
		}
		else
		{
			if ( backgroundFile < background_file_list.size() && backgroundFile >= 0 && background_file_list.size() > 0 )
				paint_helper->paintWeatherIcon( p, day_icon_rect[i], data_day_icon[i], background_file_list.at(backgroundFile).file_name, true );
		}

		// day name
		QString day_name = "";
		if ( dayNamesSystem == 0 ) day_name = data_day_name[i];
		else if ( dayNamesSystem == 1 ) day_name = QDate::shortDayName( QDate::currentDate().addDays( i ).dayOfWeek() );
		else if ( dayNamesSystem == 2 ) day_name = QDate::longDayName( QDate::currentDate().addDays( i ).dayOfWeek() );
		else if ( dayNamesSystem == 3 ) day_name = QDate::currentDate().addDays( i ).toString( Qt::DefaultLocaleShortDate );

		if ( day_name != "" )
		{
			p->setFont( paint_helper->getFittingFontSize( p, day_name_rect[i], Qt::AlignVCenter | Qt::AlignHCenter,
				day_name, day_name_font, true, true, 0.3, 0, 0 ) );

			paint_helper->drawTextWithShadow( p, day_name_rect[i], Qt::AlignVCenter | Qt::AlignHCenter, day_name );
		}

		// temperature low/high
		QString temp_low_high = "";
		if ( data_day_temperature_high[i] != "" && data_day_temperature_high[i].contains(QRegExp("[0-9]")) )
		{
			temp_low_high = temp_low_high + data_day_temperature_high[i] + QChar(0xB0) + ( showTemperatureUnit ? tempType : "" );
		}
		if ( data_day_temperature_high[i] != "" && data_day_temperature_low[i] != "" && data_day_temperature_high[i].contains(QRegExp("[0-9]")) &&
			data_day_temperature_low[i].contains(QRegExp("[0-9]")) )
		{
			temp_low_high = temp_low_high + " / ";
		}
		if ( data_day_temperature_low[i] != "" && data_day_temperature_low[i].contains(QRegExp("[0-9]")) )
		{
			temp_low_high = temp_low_high + data_day_temperature_low[i] + QChar(0xB0) + ( showTemperatureUnit ? tempType : "" );
		}

		p->setFont( paint_helper->getFittingFontSize( p, day_temp_rect[i], Qt::AlignVCenter | Qt::AlignHCenter,
			temp_low_high, day_temperature_font, true, true, 0.3, 0, 0 ) );

		paint_helper->drawTextWithShadow( p, day_temp_rect[i], Qt::AlignVCenter | Qt::AlignHCenter, temp_low_high );

		// icon text
		QString number;
		number.setNum( i+1 );

		if ( !omitIconDescription )
		{
			p->setFont( paint_helper->getFittingFontSize( p, day_icon_text_rect[i], Qt::AlignBottom | Qt::AlignHCenter | Qt::TextWordWrap,
				data_day_icon_text[i], day_icon_text_font, false, true, 0.2, 0, 0 ) );
			paint_helper->drawTextWithShadow( p, day_icon_text_rect[i], Qt::AlignBottom | Qt::AlignHCenter | Qt::TextWordWrap, data_day_icon_text[i] );
		}
	}

	p->end();
	delete p;
	p = NULL;

	if ( allow_config_saving )
	{
		KConfigGroup cg = config();
		cg.writeEntry( "graphics_widget_size", graphics_widget->size() );
		emit configNeedsSaving();
	}

	updateTooltip();
	updatePopupIcon();

	graphics_widget->update();
	update();

	updating = false;
}

void Plasma_CWP::createConfigurationInterface(KConfigDialog* parent)
{
	layoutSizeBeforeConfig = size();
	layoutSizeBeforeConfigGraphicsWidget = graphics_widget->size();

	bool error = false;
	conf = new ConfigDialog();
	parent->setButtons( KDialog::Ok | KDialog::Cancel | KDialog::Apply );
	parent->addPage( conf, parent->windowTitle(), icon() );

	conf->layoutComboBox->addItem( i18n("1 Day - horizontal") );
	conf->layoutComboBox->addItem( i18n("1 Day - vertical") );
	conf->layoutComboBox->addItem( i18n("3 Days - horizontal") );
	conf->layoutComboBox->addItem( i18n("3 Days - vertical") );
	conf->layoutComboBox->addItem( i18n("5 Days - horizontal") );
	conf->layoutComboBox->addItem( i18n("5 Days - horizontal (compact)") );
	conf->layoutComboBox->addItem( i18n("5 Days - vertical") );
	conf->layoutComboBox->addItem( i18n("7 Days - horizontal") );
	conf->layoutComboBox->addItem( i18n("7 Days - horizontal (compact)") );
	conf->layoutComboBox->addItem( i18n("7 Days - vertical") );
	conf->layoutComboBox->addItem( i18n("4 Days - custom image - horizontal") );
	conf->layoutComboBox->addItem( i18n("4 Days - custom image - vertical") );
	conf->layoutComboBox->addItem( i18n("7 Days - custom image") );

	conf->customImageKUrlRequester->setFilter( "image/png image/jpeg image/gif" );
// causes crash on mandriva:
//	 conf->customImageKUrlRequester->setMode( KFile::Files | KFile::ExistingOnly | KFile::LocalOnly );

	conf->iconsCustomKUrlRequester->setFilter( "image/svg+xml image/svg+xml-compressed" );

	int i;
	for ( i=0; i<xml_data_file_list.size(); i++ )
	{
		conf->xmlDataFileComboBox->addItem( xml_data_file_list.at(i).name + " (" + QChar(0xB0) + xml_data_file_list.at(i).unit + ") - " + xml_data_file_list.at(i).version );
	}
	for ( i=0; i<background_file_list.size(); i++ )
	{
		conf->backgroundFileComboBox->addItem( background_file_list.at(i).name );
	}

	conf->xmlDataFileComboBox->setCurrentIndex( xmlDataFile );
	conf->zipText->setText(zip);
	conf->freqChooser->setValue( updateFrequency.toInt( &error ) );
	conf->updateTimeCheckBox->setChecked( updateTime );
	conf->showTemperatureUnitCheckBox->setChecked( showTemperatureUnit );

	conf->layoutComboBox->setCurrentIndex( layoutNumber );
	if ( icons == 1 ) conf->iconsKDESelect->setChecked( true );
	else if ( icons == 2 ) conf->iconsCustomSelect->setChecked( true );
	else conf->iconsProviderSelect->setChecked( true );
	conf->iconsCustomKUrlRequester->setUrl( iconsCustom );
	if ( forecastSeparator == 1 ) conf->forecastSeparatorCustomSelect->setChecked( true );
	else if ( forecastSeparator == 2 ) conf->forecastSeparatorPlasmaSelect->setChecked( true );
	else conf->forecastSeparatorNoneSelect->setChecked( true );
	conf->customImageKUrlRequester->setUrl( KUrl( customImageList.size() > 0 ? customImageList.at( 0 ) : "" ) );
	conf->backgroundFileComboBox->setCurrentIndex( backgroundFile );
	conf->iconsScaleDoubleSpinBox->setValue( scaleIcons );
	conf->feelsLikeText->setText( feelsLike );
	conf->humidityText->setText( humidity );
	conf->windText->setText( wind );
	conf->windRoseText->setText( windRose );
	conf->invertWindRoseCheckBox->setChecked( invertWindRose );
	conf->windRoseScaleDoubleSpinBox->setValue( windRoseScale );
	conf->rainText->setText( rain );
	conf->dewPointText->setText( dewPoint );
	conf->visibilityText->setText( visibility );
	conf->pressureText->setText( pressure );
	conf->uvIndexText->setText( uvIndex );

	if ( background == 1 ) conf->translucentBackgroundSelect->setChecked( true );
	else if ( background == 2 ) conf->noBackgroundSelect->setChecked( true );
	else if ( background == 3 ) conf->customColorBackgroundSelect->setChecked( true );
	else conf->defaultBackgroundSelect->setChecked( true );
	conf->customColorKColorButton->setColor( backgroundColor );
	conf->transparencySlider->setValue( backgroundTransparency );
	conf->fontComboBox->setCurrentFont( font );
	conf->fontKColorButton->setColor( fontColor );
	conf->fontShadowCheckBox->setChecked( fontShadow );
	conf->fontScaleDoubleSpinBox->setValue( scaleFont );

	conf->omitIconDescriptionCheckBox->setChecked( omitIconDescription );
	if ( dayNamesSystem == 1 ) conf->dayNamesSystemShortSelect->setChecked( true );
	else if ( dayNamesSystem == 2 ) conf->dayNamesSystemLongSelect->setChecked( true );
	else if ( dayNamesSystem == 3 ) conf->dayNamesSystemDateSelect->setChecked( true );
	else conf->dayNamesProviderSelect->setChecked( true );

	if ( windLayoutRose ) conf->windLayoutRoseSelect->setChecked( true );
	else conf->windLayoutProviderSelect->setChecked( true );

	connect( conf->importSettingsPushButton, SIGNAL(clicked()), this, SLOT(importSettings()) );
	connect( conf->exportSettingsPushButton, SIGNAL(clicked()), this, SLOT(exportSettings()) );

	connect( conf->xmlDataFileComboBox, SIGNAL(activated(int)), this, SLOT(xmlDataFileSelected(int)) );
	xmlDataFileSelected( xmlDataFile );

	connect( conf->preferredLocationComboBox, SIGNAL(activated(int)), this, SLOT(preferredLocationSelected(int)) );
	connect( conf->newPreferredLocationPushButton, SIGNAL(clicked()), this, SLOT(preferredLocationNew()) );
	connect( conf->savePreferredLocationPushButton, SIGNAL(clicked()), this, SLOT(preferredLocationSave()) );
	connect( conf->editNamePreferredLocationPushButton, SIGNAL(clicked()), this, SLOT(preferredLocationEditName()) );
	connect( conf->deletePreferredLocationPushButton, SIGNAL(clicked()), this, SLOT(preferredLocationRemove()) );

	connect( conf->customImageKUrlRequester, SIGNAL(textChanged( const QString & )), this, SLOT(customImageUrlChanged( const QString & )) );
	connect( conf->customImageListComboBox, SIGNAL(activated(int)), this, SLOT(customImageListSelected( int )) );
	connect( conf->newCustomImagePushButton, SIGNAL(clicked()), this, SLOT(customImageNew()) );
	connect( conf->saveCustomImagePushButton, SIGNAL(clicked()), this, SLOT(customImageSave()) );
	connect( conf->editNameCustomImagePushButton, SIGNAL(clicked()), this, SLOT(customImageEditName()) );
	connect( conf->deleteCustomImagePushButton, SIGNAL(clicked()), this, SLOT(customImageRemove()) );

	connect( conf->zipText, SIGNAL( textChanged( const QString & ) ), this, SLOT( zipTextChanged( const QString & ) ) );

	connect( parent, SIGNAL(applyClicked()), this, SLOT(configAccepted()) );
	connect( parent, SIGNAL(okClicked()), this, SLOT(configAccepted()) );
	connect( parent, SIGNAL(cancelClicked()), this, SLOT(configRejected()) );

	connect( conf->satelliteImagePushButton, SIGNAL( clicked() ), this, SLOT( satelliteImagePushButtonPressed() ) );

	connect( conf->xmlDataFileComboBox, SIGNAL( activated( int ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->zipText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->updateTimeCheckBox, SIGNAL( stateChanged( int ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->showTemperatureUnitCheckBox, SIGNAL( stateChanged( int ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->layoutComboBox, SIGNAL( activated( int ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->iconsProviderSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->iconsKDESelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->iconsCustomSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->iconsCustomKUrlRequester, SIGNAL( urlSelected( const KUrl & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->iconsCustomKUrlRequester, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->forecastSeparatorNoneSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->forecastSeparatorCustomSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->forecastSeparatorPlasmaSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->customImageKUrlRequester, SIGNAL( urlSelected( const KUrl & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->customImageKUrlRequester, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->backgroundFileComboBox, SIGNAL( activated( int ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->iconsScaleDoubleSpinBox, SIGNAL( valueChanged( double ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->feelsLikeText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->humidityText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->windText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->windRoseText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->invertWindRoseCheckBox, SIGNAL( stateChanged( int ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->windRoseScaleDoubleSpinBox, SIGNAL( valueChanged( double ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->rainText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->dewPointText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->visibilityText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->pressureText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->uvIndexText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->defaultBackgroundSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->translucentBackgroundSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->noBackgroundSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->customColorBackgroundSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->customColorKColorButton, SIGNAL( changed( const QColor & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->defaultBackgroundSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->transparencySlider, SIGNAL( sliderMoved( int ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->fontComboBox, SIGNAL( activated( int ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->fontKColorButton, SIGNAL( changed( const QColor & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->fontShadowCheckBox, SIGNAL( stateChanged( int ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->fontScaleDoubleSpinBox, SIGNAL( valueChanged( double ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->omitIconDescriptionCheckBox, SIGNAL( stateChanged( int ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->dayNamesSystemShortSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->dayNamesSystemLongSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->dayNamesSystemDateSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->dayNamesProviderSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->windLayoutRoseSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->windLayoutProviderSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );

	for ( i=0; i<customImageList.size() && i<customImageNameList.size(); i++ )
	{
		if ( customImageNameList.at( i ) != "" ) conf->customImageListComboBox->addItem( customImageNameList.at( i ) );
	}

	if ( customImageCurrent >= 0 && customImageCurrent <= customImageNameList.size() - 1 )
	{
		conf->customImageListComboBox->setCurrentIndex( customImageCurrent );
	}

	if ( customImageCurrent >= 0 && customImageCurrent <= customImageList.size() - 1 )
	{
		conf->customImageKUrlRequester->setUrl( KUrl( customImageList.at( customImageCurrent ) ) );
	}

	for ( i=0; i<preferred_location_list.size(); i++ )
	{
		conf->preferredLocationComboBox->addItem( preferred_location_list.at(i).name );
	}

	for ( i=0; i<preferred_location_list.size(); i++ )
	{
		if ( preferred_location_list.at(i).weather_provider == conf->xmlDataFileComboBox->currentIndex() &&
			 preferred_location_list.at(i).zip == conf->zipText->text() &&
			 preferred_location_list.at(i).custom_image_list == customImageList &&
			 preferred_location_list.at(i).custom_image_name_list == customImageNameList )
		{
			conf->preferredLocationComboBox->setCurrentIndex( i );
		}
	}

	if ( preferred_location_list.size() > 0 && conf->preferredLocationComboBox->currentIndex() >= 0 &&
		conf->preferredLocationComboBox->currentIndex() + 1 < preferred_location_list.size() )
	{
		if ( preferred_location_list.at(conf->preferredLocationComboBox->currentIndex()).weather_provider != conf->xmlDataFileComboBox->currentIndex() ||
			 preferred_location_list.at(conf->preferredLocationComboBox->currentIndex()).zip != conf->zipText->text() ||
			 preferred_location_list.at(conf->preferredLocationComboBox->currentIndex()).custom_image_list != customImageList ||
			 preferred_location_list.at(conf->preferredLocationComboBox->currentIndex()).custom_image_name_list != customImageNameList )
		{
			conf->preferredLocationComboBox->setItemText( conf->preferredLocationComboBox->currentIndex(),
				preferred_location_list.at(conf->preferredLocationComboBox->currentIndex()).name + " *" );
		}
	}
}

void Plasma_CWP::configAppearanceChanged()
{
	if ( !conf ) return;

	if ( customImageList.size() == 1 && customImageNameList.size() == 1 && customImageNameList.at( 0 ) == "" )
	{
		customImageList.clear();
		customImageNameList.clear();
	}

	if ( conf->customImageKUrlRequester->url().url() != "" && customImageList.size() == 0 )
	{
		customImageList.prepend( conf->customImageKUrlRequester->url().url() );
		customImageNameList.prepend( "" );
	}

	if ( customImageCurrent < 0 ) customImageCurrent = 0;
	if ( customImageCurrent > customImageList.size() - 1 ) customImageCurrent = customImageList.size() - 1;

	int xmlDataFile_old = xmlDataFile;
	QString zip_old = zip;

	xmlDataFile = conf->xmlDataFileComboBox->currentIndex();
	zip = conf->zipText->text();
	updateFrequency.setNum(conf->freqChooser->value());
	updateTime = conf->updateTimeCheckBox->isChecked();
	showTemperatureUnit = conf->showTemperatureUnitCheckBox->isChecked();

	layoutNumber = conf->layoutComboBox->currentIndex();
	icons = conf->iconsKDESelect->isChecked() ? 1 : ( conf->iconsCustomSelect->isChecked() ? 2 : 0 );
	iconsCustom = conf->iconsCustomKUrlRequester->url();
	forecastSeparator = conf->forecastSeparatorCustomSelect->isChecked() ? 1 : ( conf->forecastSeparatorPlasmaSelect->isChecked() ? 2 : 0 );
	backgroundFile = conf->backgroundFileComboBox->currentIndex();
	scaleIcons = conf->iconsScaleDoubleSpinBox->value();
	feelsLike = conf->feelsLikeText->text();
	humidity = conf->humidityText->text();
	wind = conf->windText->text();
	windRose = conf->windRoseText->text();
	invertWindRose = conf->invertWindRoseCheckBox->isChecked();
	windRoseScale = conf->windRoseScaleDoubleSpinBox->value();
	rain = conf->rainText->text();
	dewPoint = conf->dewPointText->text();
	visibility = conf->visibilityText->text();
	pressure = conf->pressureText->text();
	uvIndex = conf->uvIndexText->text();

	background = conf->translucentBackgroundSelect->isChecked() ? 1 : ( conf->noBackgroundSelect->isChecked() ? 2 : ( conf->customColorBackgroundSelect->isChecked() ? 3: 0 ) );
	backgroundColor = conf->customColorKColorButton->color();
	backgroundTransparency = conf->transparencySlider->value();
	font = conf->fontComboBox->currentFont().toString();
	fontColor = conf->fontKColorButton->color();
	fontShadow = conf->fontShadowCheckBox->isChecked();
	scaleFont = conf->fontScaleDoubleSpinBox->value();

	omitIconDescription = conf->omitIconDescriptionCheckBox->isChecked();
	if ( conf->dayNamesProviderSelect->isChecked() ) dayNamesSystem = 0;
	else if ( conf->dayNamesSystemShortSelect->isChecked() ) dayNamesSystem = 1;
	else if ( conf->dayNamesSystemLongSelect->isChecked() ) dayNamesSystem = 2;
	else dayNamesSystem = 3;

	windLayoutRose = conf->windLayoutRoseSelect->isChecked();

	if ( size().height() < 150. )
	{
		setBackgroundHints( Applet::NoBackground );
	}
	else
	{
		if ( background == 1 ) // translucent
			setBackgroundHints( Applet::TranslucentBackground );
		else if ( background == 2 ) // none
			setBackgroundHints( Applet::NoBackground );
		else if ( background == 3 ) // custom
			setBackgroundHints( Applet::NoBackground );
		else
			setBackgroundHints( Applet::DefaultBackground );
	}

	if ( data_provider )
	{
		data_provider->get_weather_values( data_location_location, data_location_country, data_sun_sunrise,
			data_sun_sunset, data_current_temperature, data_current_temperature_felt, data_current_wind_code, data_current_wind_speed, data_current_wind,
			data_current_humidity, data_current_dew_point, data_current_visibility, data_current_pressure, data_current_uv_index,
			data_current_icon_code, data_current_icon, data_current_icon_text, data_current_rain,
			&data_day_name[0], &data_day_temperature_high[0], &data_day_temperature_low[0], &data_day_icon_code[0], &data_day_icon[0],
			&data_day_icon_text[0], data_update_time, data_provider_update_time, tempType, data_custom_image_list,
			last_download_succeeded, provider_url );

		KConfigGroup cg = config();
		cg.writeEntry( "ProviderURL", provider_url );
		emit configNeedsSaving();

		paint_helper->setValues( humidity, wind, invertWindRose, windRoseScale, rain, dewPoint, visibility,
			pressure, uvIndex, tempType, icons, iconsCustom, background, backgroundColor,
			backgroundTransparency, scaleIcons, font, fontColor, fontShadow, scaleFont,
			forecastSeparator, layoutDirection(), data_custom_image_list.size() > 0 ? data_custom_image_list.at ( 0 ) : QByteArray(),
			data_current_wind, data_current_humidity, data_current_rain, data_current_dew_point,
			data_current_visibility, data_current_pressure, data_current_uv_index );
	}
	custom_image_dialog->setFont( font );

	updateGraphicsWidgetDelayed();
	updatePopupIcon();

	QList<KUrl> tmp_url_list;
	int i;
	for ( i=0; i<customImageList.size(); i++ )
	{
		tmp_url_list.append( KUrl( customImageList.at( i ) ) );
	}

	if ( data_provider )
	{
		if ( xmlDataFile < xml_data_file_list.size() && xmlDataFile >= 0 && xml_data_file_list.size() > 0 )
			data_provider->set_config_values( updateFrequency, xml_data_file_list.at(xmlDataFile).file_name, zip,
				feelsLike, humidity, wind, tempType, tmp_url_list );

		if ( zip != zip_old || xmlDataFile != xmlDataFile_old ) emit( reloadRequested() );
	}

	update_timer->stop();
	bool error;
	update_timer->start( updateFrequency.toInt( &error )*60*1000 ); // update every x minutes
	QTimer::singleShot( 10*1000, this, SLOT( reloadData() ) );
}

void Plasma_CWP::configAccepted()
{
	if ( customImageList.size() == 1 && customImageNameList.size() == 1 && customImageNameList.at( 0 ) == "" )
	{
		customImageList.clear();
		customImageNameList.clear();
	}

	if ( conf->customImageKUrlRequester->url().url() != "" && customImageList.size() == 0 )
	{
		customImageList.prepend( conf->customImageKUrlRequester->url().url() );
		customImageNameList.prepend( "" );
	}

	if ( customImageCurrent < 0 ) customImageCurrent = 0;
	if ( customImageCurrent > customImageList.size() - 1 ) customImageCurrent = customImageList.size() - 1;

	KConfigGroup cg = config();
	cg.writeEntry( "cwpIdentifier", cwpIdentifier );
	cg.writeEntry( "xmlDataFile", conf->xmlDataFileComboBox->currentIndex() );
	cg.writeEntry( "zip", conf->zipText->text() );
	cg.writeEntry( "updateFrequency", updateFrequency.setNum( conf->freqChooser->value() ) );
	cg.writeEntry( "updateTime", conf->updateTimeCheckBox->isChecked() );
	cg.writeEntry( "showTemperatureUnit", conf->showTemperatureUnitCheckBox->isChecked() );

	cg.writeEntry( "provider_update_time_shown", provider_update_time_shown );

	cg.writeEntry( "layoutNumber", conf->layoutComboBox->currentIndex());
	cg.writeEntry( "icons", conf->iconsKDESelect->isChecked() ? 1 : ( conf->iconsCustomSelect->isChecked() ? 2 : 0 ) );
	cg.writeEntry( "iconsCustom", conf->iconsCustomKUrlRequester->url() );
	cg.writeEntry( "forecastSeparator", conf->forecastSeparatorCustomSelect->isChecked() ? 1 : ( conf->forecastSeparatorPlasmaSelect->isChecked() ? 2 : 0 ) );
	cg.writeEntry( "customImageList", customImageList );
	cg.writeEntry( "customImageNameList", customImageNameList );

	// remove pre-0.9.8 stuff
	cg.deleteEntry( "locationImage" );

	cg.writeEntry( "customImageCurrent", customImageCurrent );
	cg.writeEntry( "backgroundFile", conf->backgroundFileComboBox->currentIndex() );
	cg.writeEntry( "scaleIcons", conf->iconsScaleDoubleSpinBox->value() );
	cg.writeEntry( "feelsLike", conf->feelsLikeText->text() );
	cg.writeEntry( "humidity", conf->humidityText->text() );
	cg.writeEntry( "wind", conf->windText->text() );
	cg.writeEntry( "windRose", conf->windRoseText->text() );
	cg.writeEntry( "invertWindRose", conf->invertWindRoseCheckBox->isChecked() );
	cg.writeEntry( "windRoseScale", conf->windRoseScaleDoubleSpinBox->value() );
	cg.writeEntry( "rain", conf->rainText->text() );
	cg.writeEntry( "dewPoint", conf->dewPointText->text() );
	cg.writeEntry( "visibility", conf->visibilityText->text() );
	cg.writeEntry( "pressure", conf->pressureText->text() );
	cg.writeEntry( "uvIndex", conf->uvIndexText->text() );

	cg.writeEntry( "background", conf->translucentBackgroundSelect->isChecked() ? 1 : ( conf->noBackgroundSelect->isChecked() ? 2 : ( conf->customColorBackgroundSelect->isChecked() ? 3 : 0 ) ) );
	cg.writeEntry( "backgroundColor", conf->customColorKColorButton->color() );
	cg.writeEntry( "backgroundTransparency", conf->transparencySlider->value() );
	cg.writeEntry( "font", conf->fontComboBox->currentFont() );
	cg.writeEntry( "fontColor", conf->fontKColorButton->color() );
	cg.writeEntry( "fontShadow", conf->fontShadowCheckBox->isChecked() );
	cg.writeEntry( "scaleFont", conf->fontScaleDoubleSpinBox->value());

	cg.writeEntry( "omitIconDescription", conf->omitIconDescriptionCheckBox->isChecked() );

	if ( conf->dayNamesProviderSelect->isChecked() ) cg.writeEntry( "dayNamesSystem", 0 );
	else if ( conf->dayNamesSystemShortSelect->isChecked() ) cg.writeEntry( "dayNamesSystem", 1 );
	else if ( conf->dayNamesSystemLongSelect->isChecked() ) cg.writeEntry( "dayNamesSystem", 2 );
	else cg.writeEntry( "dayNamesSystem", 3 );

	cg.writeEntry( "windLayoutRose", conf->windLayoutRoseSelect->isChecked() );

	QList<int> index_list;
	QStringList name_list;
	QList<int> weather_provider_list;
	QStringList zip_list;
	int i;
	for ( i=0; i<preferred_location_list.size(); i++ )
	{
		index_list << preferred_location_list.at(i).index;
		name_list << preferred_location_list.at(i).name;
		weather_provider_list << preferred_location_list.at(i).weather_provider;
		zip_list << preferred_location_list.at(i).zip;
		cg.writeEntry( QString( "preferredLocationCustomImageList%1" ).arg( i ), preferred_location_list.at(i).custom_image_list );
		cg.writeEntry( QString( "preferredLocationCustomImageNameList%1" ).arg( i ), preferred_location_list.at(i).custom_image_name_list );
	}
	cg.writeEntry( "preferredLocationIndexList", index_list );
	cg.writeEntry( "preferredLocationNameList", name_list );
	cg.writeEntry( "preferredLocationWeatherProviderList", weather_provider_list );
	cg.writeEntry( "preferredLocationZipList", zip_list );

	emit configNeedsSaving();

	emit( refreshRequested() );

	QList<KUrl> tmp_url_list;
	for ( i=0; i<customImageList.size(); i++ )
	{
		tmp_url_list.append( KUrl( customImageList.at( i ) ) );
	}

	if ( data_provider )
	{
		if ( xmlDataFile < xml_data_file_list.size() && xmlDataFile >= 0 && xml_data_file_list.size() > 0 )
			data_provider->set_config_values( updateFrequency, xml_data_file_list.at(xmlDataFile).file_name, zip,
				feelsLike, humidity, wind, tempType, tmp_url_list );
		emit ( reloadRequested() );
	}


	update_timer->stop();
	bool error;
	update_timer->start( updateFrequency.toInt( &error )*60*1000 ); // update every x minutes
	QTimer::singleShot( 10*1000, this, SLOT( reloadData() ) );

	delete conf;
	conf = NULL;

	createMenu();
}

void Plasma_CWP::configRejected()
{
	resize( layoutSizeBeforeConfig );
	graphics_widget->resize( layoutSizeBeforeConfigGraphicsWidget );

	delete conf;
	conf = NULL;

	emit( refreshRequested() );
}

void Plasma_CWP::importSettings()
{
	KUrl settingsfileurl = KFileDialog::getOpenUrl( KUrl(), "*.cwp" );
	if ( settingsfileurl.isEmpty() ) // no file chosen
	{
		qDebug() << "No settings file selected, aborting.";
		return;
	}
	QFile settingsfile( settingsfileurl.path() );
	settingsfile.open( QIODevice::ReadOnly );
	if ( !settingsfile.isOpen() )
	{
		qDebug() << "Could not open settings file " << settingsfileurl;
		KMessageBox::information( 0, i18n( "Could not open CWP settings file %1." ).arg( settingsfileurl.path() ) );
		return;
	}
	if ( !settingsfile.exists() )
	{
		qDebug() << "File " << settingsfileurl << " does not exist!";
		// Don't display a warning dialog here as it is virtually impossible to open a non-existing file.
		return;
	}
	settingsfile.close();

	if ( KMessageBox::questionYesNo( 0, i18n( "Loading CWP settings from file.\nThis will overwrite the current configuration. Continue?" ) ) == KMessageBox::No ) return;

	KConfig cf( settingsfileurl.path(), KConfig::SimpleConfig );
	KConfigGroup cg( &cf, "" );

	// copy and paste from readConfigData()
	cwpIdentifier = cg.readEntry( "cwpIdentifier", 0 );
	xmlDataFile = cg.readEntry( "xmlDataFile", 0 );
	zip = cg.readEntry( "zip", "BRXX0043");
	updateFrequency = cg.readEntry( "updateFrequency", "5" );
	updateTime = cg.readEntry( "updateTime", true );
	showTemperatureUnit = cg.readEntry( "showTemperatureUnit", true );

	provider_update_time_shown = cg.readEntry( "provider_update_time_shown", false );

	layoutNumber = cg.readEntry( "layoutNumber", 0 );
	icons = cg.readEntry( "icons", 1 );
	iconsCustom = cg.readEntry( "iconsCustom", "" );
	backgroundFile = cg.readEntry( "backgroundFile", 0 );
	scaleIcons = cg.readEntry( "scaleIcons", 1.3 );
	forecastSeparator = cg.readEntry( "forecastSeparator", 1 );
	customImageList = cg.readEntry( "customImageList", QStringList() );
	customImageNameList = cg.readEntry( "customImageNameList", QStringList() );

	if ( customImageList.size() <= 0 || customImageNameList.size() <= 0 )
	{
		customImageList.clear();
		customImageNameList.clear();
		QString custom_image_pre_0_9_8 = cg.readEntry( "locationImage", "" );
		if ( custom_image_pre_0_9_8 != "" )
		{
			customImageList.append( custom_image_pre_0_9_8 );
			customImageNameList.append( "" );
		}
	}

	customImageCurrent = cg.readEntry( "customImageCurrent", 0 );
	feelsLike = cg.readEntry( "feelsLike", i18nc( "Due to wind chill, temperature is felt like", "feels like" ) );
	humidity = cg.readEntry( "humidity", i18n( "Humidity" ) );
	wind = cg.readEntry( "wind", i18n( "Wind" ) );
	windRose = cg.readEntry( "windRose", i18nc( "Wind directions: North, South, East, West. Separated by \";\", no spaces!", "N;S;E;W;" ) );
	invertWindRose = cg.readEntry( "invertWindRose", false );
	windRoseScale = cg.readEntry( "windRoseScale", 0.9 );
	rain = cg.readEntry( "rain", i18n( "Rain" ) );
	dewPoint = cg.readEntry( "dewPoint", i18n( "Dew Point" ) );
	visibility = cg.readEntry( "visibility", i18n( "Visibility" ) );
	pressure = cg.readEntry( "pressure", i18n( "Pressure" ) );
	uvIndex = cg.readEntry( "uvIndex", i18n( "UV Index" ) );

	background = cg.readEntry( "background", 1 );
	backgroundColor = cg.readEntry( "backgroundColor", Plasma::Theme::defaultTheme()->color( Plasma::Theme::BackgroundColor ) );
	backgroundTransparency = cg.readEntry( "backgroundTransparency", 224 );
	font = cg.readEntry( "font", Plasma::Theme::defaultTheme()->font( Plasma::Theme::DefaultFont ).toString() );
	fontColor = cg.readEntry( "fontColor", Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
	fontShadow = cg.readEntry( "fontShadow", true );
	scaleFont = cg.readEntry( "scaleFont", 1.0 );

	omitIconDescription = cg.readEntry( "omitIconDescription", false );
	dayNamesSystem = cg.readEntry( "dayNamesSystem", 1 );

	windLayoutRose = cg.readEntry( "windLayoutRose", true );

	provider_url = cg.readEntry( "ProviderURL", "" );

	QList<int> index_list = cg.readEntry( "preferredLocationIndexList", QList<int>() );
	QStringList name_list = cg.readEntry( "preferredLocationNameList", QStringList() );
	QList<int> weather_provider_list = cg.readEntry( "preferredLocationWeatherProviderList", QList<int>() );
	QStringList zip_list = cg.readEntry( "preferredLocationZipList", QStringList() );

	int i;
	if ( index_list.size() == name_list.size() && index_list.size() == weather_provider_list.size() && index_list.size() == zip_list.size() )
	{
		preferred_location_list.clear();
		PreferredLocation tmp_location;
		for ( i=0; i<index_list.size(); i++ )
		{
			tmp_location.index = index_list.at(i);
			tmp_location.name = name_list.at(i);
			tmp_location.weather_provider = weather_provider_list.at(i);
			tmp_location.zip = zip_list.at(i);

			preferred_location_list.append( tmp_location );
		}
	}
	for ( i=0; i<25; i++ )
	{
		QStringList custom_image_list = cg.readEntry( QString( "preferredLocationCustomImageList%1" ).arg( i ), QStringList() );
		QStringList custom_image_name_list = cg.readEntry( QString( "preferredLocationCustomImageNameList%1" ).arg( i ), QStringList() );

		if ( custom_image_list.size() == custom_image_name_list.size() && custom_image_list.size() > 0 )
		{
			if ( i < preferred_location_list.size() )
			{
				preferred_location_list[i].custom_image_list = custom_image_list;
				preferred_location_list[i].custom_image_name_list = custom_image_name_list;
			}
		}
	}

	// size used for font scale on tooltip
	QSizeF size_tmp = cg.readEntry( "graphics_widget_size", QSizeF( 0., 0. ) );
	if ( size_tmp.width() >= 150. && size_tmp.height() >= 150. )
	{
		graphics_widget->resize( size_tmp );
		graphics_widget->getPixmap() = graphics_widget->getPixmap().scaled( size_tmp.toSize() );
	}

	size_tmp = cg.readEntry( "plasmoid_size", QSizeF( 0., 0. ) );
	if ( size_tmp.width() >= 150. && size_tmp.height() >= 150. )
	{
		resize( size_tmp );
	}

	QSize custom_image_dialog_size = cg.readEntry( "custom_image_dialog_size", QSize( 0, 0 ) );
	if ( custom_image_dialog_size.width() != 0 && custom_image_dialog_size.height() != 0 )
	{
		custom_image_dialog->resize( custom_image_dialog_size );
	}

	if ( size().height() < 150. )
	{
		setBackgroundHints( Applet::NoBackground );
	}
	else
	{
		if ( background == 1 ) // translucent
			setBackgroundHints( Applet::TranslucentBackground );
		else if ( background == 2 ) // none
			setBackgroundHints( Applet::NoBackground );
		else if ( background == 3 ) // custom
			setBackgroundHints( Applet::NoBackground );
		else
			setBackgroundHints( Applet::DefaultBackground );
	}
	layoutSizeBeforeConfig = size();
	layoutSizeBeforeConfigGraphicsWidget = graphics_widget->size();

	disconnect( conf->importSettingsPushButton, SIGNAL(clicked()), this, SLOT(importSettings()) );
	disconnect( conf->exportSettingsPushButton, SIGNAL(clicked()), this, SLOT(exportSettings()) );

	disconnect( conf->xmlDataFileComboBox, SIGNAL(activated(int)), this, SLOT(xmlDataFileSelected(int)) );

	disconnect( conf->preferredLocationComboBox, SIGNAL(activated(int)), this, SLOT(preferredLocationSelected(int)) );
	disconnect( conf->newPreferredLocationPushButton, SIGNAL(clicked()), this, SLOT(preferredLocationNew()) );
	disconnect( conf->savePreferredLocationPushButton, SIGNAL(clicked()), this, SLOT(preferredLocationSave()) );
	disconnect( conf->editNamePreferredLocationPushButton, SIGNAL(clicked()), this, SLOT(preferredLocationEditName()) );
	disconnect( conf->deletePreferredLocationPushButton, SIGNAL(clicked()), this, SLOT(preferredLocationRemove()) );

	disconnect( conf->customImageKUrlRequester, SIGNAL(textChanged( const QString & )), this, SLOT(customImageUrlChanged( const QString & )) );
	disconnect( conf->customImageListComboBox, SIGNAL(activated(int)), this, SLOT(customImageListSelected( int )) );
	disconnect( conf->newCustomImagePushButton, SIGNAL(clicked()), this, SLOT(customImageNew()) );
	disconnect( conf->saveCustomImagePushButton, SIGNAL(clicked()), this, SLOT(customImageSave()) );
	disconnect( conf->editNameCustomImagePushButton, SIGNAL(clicked()), this, SLOT(customImageEditName()) );
	disconnect( conf->deleteCustomImagePushButton, SIGNAL(clicked()), this, SLOT(customImageRemove()) );

	disconnect( conf->zipText, SIGNAL( textChanged( const QString & ) ), this, SLOT( zipTextChanged( const QString & ) ) );

	disconnect( conf->satelliteImagePushButton, SIGNAL( clicked() ), this, SLOT( satelliteImagePushButtonPressed() ) );

	disconnect( conf->xmlDataFileComboBox, SIGNAL( activated( int ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->zipText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->updateTimeCheckBox, SIGNAL( stateChanged( int ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->showTemperatureUnitCheckBox, SIGNAL( stateChanged( int ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->layoutComboBox, SIGNAL( activated( int ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->iconsProviderSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->iconsKDESelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->iconsCustomSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->iconsCustomKUrlRequester, SIGNAL( urlSelected( const KUrl & ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->iconsCustomKUrlRequester, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->forecastSeparatorNoneSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->forecastSeparatorCustomSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->forecastSeparatorPlasmaSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->customImageKUrlRequester, SIGNAL( urlSelected( const KUrl & ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->customImageKUrlRequester, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->backgroundFileComboBox, SIGNAL( activated( int ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->iconsScaleDoubleSpinBox, SIGNAL( valueChanged( double ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->feelsLikeText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->humidityText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->windText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->windRoseText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->invertWindRoseCheckBox, SIGNAL( stateChanged( int ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->windRoseScaleDoubleSpinBox, SIGNAL( valueChanged( double ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->rainText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->dewPointText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->visibilityText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->pressureText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->uvIndexText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->defaultBackgroundSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->translucentBackgroundSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->noBackgroundSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->customColorBackgroundSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->customColorKColorButton, SIGNAL( changed( const QColor & ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->defaultBackgroundSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->transparencySlider, SIGNAL( sliderMoved( int ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->fontComboBox, SIGNAL( activated( int ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->fontKColorButton, SIGNAL( changed( const QColor & ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->fontShadowCheckBox, SIGNAL( stateChanged( int ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->fontScaleDoubleSpinBox, SIGNAL( valueChanged( double ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->omitIconDescriptionCheckBox, SIGNAL( stateChanged( int ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->dayNamesSystemShortSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->dayNamesSystemLongSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->dayNamesSystemDateSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->dayNamesProviderSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->windLayoutRoseSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	disconnect( conf->windLayoutProviderSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );


	// copy and paste from createConfigurationInterface
	bool error;
	conf->xmlDataFileComboBox->setCurrentIndex( xmlDataFile );
	conf->zipText->setText(zip);
	conf->freqChooser->setValue( updateFrequency.toInt( &error ) );
	conf->updateTimeCheckBox->setChecked( updateTime );
	conf->showTemperatureUnitCheckBox->setChecked( showTemperatureUnit );

	conf->layoutComboBox->setCurrentIndex( layoutNumber );
	if ( icons == 1 ) conf->iconsKDESelect->setChecked( true );
	else if ( icons == 2 ) conf->iconsCustomSelect->setChecked( true );
	else conf->iconsProviderSelect->setChecked( true );
	conf->iconsCustomKUrlRequester->setUrl( iconsCustom );
	if ( forecastSeparator == 1 ) conf->forecastSeparatorCustomSelect->setChecked( true );
	else if ( forecastSeparator == 2 ) conf->forecastSeparatorPlasmaSelect->setChecked( true );
	else conf->forecastSeparatorNoneSelect->setChecked( true );
	conf->customImageKUrlRequester->setUrl( KUrl( customImageList.size() > 0 ? customImageList.at( 0 ) : "" ) );
	conf->backgroundFileComboBox->setCurrentIndex( backgroundFile );
	conf->iconsScaleDoubleSpinBox->setValue( scaleIcons );
	conf->feelsLikeText->setText( feelsLike );
	conf->humidityText->setText( humidity );
	conf->windText->setText( wind );
	conf->windRoseText->setText( windRose );
	conf->invertWindRoseCheckBox->setChecked( invertWindRose );
	conf->windRoseScaleDoubleSpinBox->setValue( windRoseScale );
	conf->rainText->setText( rain );
	conf->dewPointText->setText( dewPoint );
	conf->visibilityText->setText( visibility );
	conf->pressureText->setText( pressure );
	conf->uvIndexText->setText( uvIndex );

	if ( background == 1 ) conf->translucentBackgroundSelect->setChecked( true );
	else if ( background == 2 ) conf->noBackgroundSelect->setChecked( true );
	else if ( background == 3 ) conf->customColorBackgroundSelect->setChecked( true );
	else conf->defaultBackgroundSelect->setChecked( true );
	conf->customColorKColorButton->setColor( backgroundColor );
	conf->transparencySlider->setValue( backgroundTransparency );
	conf->fontComboBox->setCurrentFont( font );
	conf->fontKColorButton->setColor( fontColor );
	conf->fontShadowCheckBox->setChecked( fontShadow );
	conf->fontScaleDoubleSpinBox->setValue( scaleFont );

	conf->omitIconDescriptionCheckBox->setChecked( omitIconDescription );
	if ( dayNamesSystem == 1 ) conf->dayNamesSystemShortSelect->setChecked( true );
	else if ( dayNamesSystem == 2 ) conf->dayNamesSystemLongSelect->setChecked( true );
	else if ( dayNamesSystem == 3 ) conf->dayNamesSystemDateSelect->setChecked( true );
	else conf->dayNamesProviderSelect->setChecked( true );

	if ( windLayoutRose ) conf->windLayoutRoseSelect->setChecked( true );
	else conf->windLayoutProviderSelect->setChecked( true );

	xmlDataFileSelected( xmlDataFile );

	conf->customImageListComboBox->clear();
	for ( i=0; i<customImageList.size() && i<customImageNameList.size(); i++ )
	{
		if ( customImageNameList.at( i ) != "" ) conf->customImageListComboBox->addItem( customImageNameList.at( i ) );
	}

	if ( customImageCurrent >= 0 && customImageCurrent <= customImageNameList.size() - 1 )
	{
		conf->customImageListComboBox->setCurrentIndex( customImageCurrent );
	}

	conf->customImageKUrlRequester->clear();
	if ( customImageCurrent >= 0 && customImageCurrent <= customImageList.size() - 1 )
	{
		conf->customImageKUrlRequester->setUrl( KUrl( customImageList.at( customImageCurrent ) ) );
	}

	conf->preferredLocationComboBox->clear();
	for ( i=0; i<preferred_location_list.size(); i++ )
	{
		conf->preferredLocationComboBox->addItem( preferred_location_list.at(i).name );
	}

	for ( i=0; i<preferred_location_list.size(); i++ )
	{
		if ( preferred_location_list.at(i).weather_provider == conf->xmlDataFileComboBox->currentIndex() &&
			 preferred_location_list.at(i).zip == conf->zipText->text() &&
			 preferred_location_list.at(i).custom_image_list == customImageList &&
			 preferred_location_list.at(i).custom_image_name_list == customImageNameList )
		{
			conf->preferredLocationComboBox->setCurrentIndex( i );
		}
	}

	if ( preferred_location_list.size() > 0 && conf->preferredLocationComboBox->currentIndex() >= 0 &&
		conf->preferredLocationComboBox->currentIndex() + 1 < preferred_location_list.size() )
	{
		if ( preferred_location_list.at(conf->preferredLocationComboBox->currentIndex()).weather_provider != conf->xmlDataFileComboBox->currentIndex() ||
			 preferred_location_list.at(conf->preferredLocationComboBox->currentIndex()).zip != conf->zipText->text() ||
			 preferred_location_list.at(conf->preferredLocationComboBox->currentIndex()).custom_image_list != customImageList ||
			 preferred_location_list.at(conf->preferredLocationComboBox->currentIndex()).custom_image_name_list != customImageNameList )
		{
			conf->preferredLocationComboBox->setItemText( conf->preferredLocationComboBox->currentIndex(),
				preferred_location_list.at(conf->preferredLocationComboBox->currentIndex()).name + " *" );
		}
	}

	connect( conf->importSettingsPushButton, SIGNAL(clicked()), this, SLOT(importSettings()) );
	connect( conf->exportSettingsPushButton, SIGNAL(clicked()), this, SLOT(exportSettings()) );

	connect( conf->xmlDataFileComboBox, SIGNAL(activated(int)), this, SLOT(xmlDataFileSelected(int)) );
	xmlDataFileSelected( xmlDataFile );

	connect( conf->preferredLocationComboBox, SIGNAL(activated(int)), this, SLOT(preferredLocationSelected(int)) );
	connect( conf->newPreferredLocationPushButton, SIGNAL(clicked()), this, SLOT(preferredLocationNew()) );
	connect( conf->savePreferredLocationPushButton, SIGNAL(clicked()), this, SLOT(preferredLocationSave()) );
	connect( conf->editNamePreferredLocationPushButton, SIGNAL(clicked()), this, SLOT(preferredLocationEditName()) );
	connect( conf->deletePreferredLocationPushButton, SIGNAL(clicked()), this, SLOT(preferredLocationRemove()) );

	connect( conf->customImageKUrlRequester, SIGNAL(textChanged( const QString & )), this, SLOT(customImageUrlChanged( const QString & )) );
	connect( conf->customImageListComboBox, SIGNAL(activated(int)), this, SLOT(customImageListSelected( int )) );
	connect( conf->newCustomImagePushButton, SIGNAL(clicked()), this, SLOT(customImageNew()) );
	connect( conf->saveCustomImagePushButton, SIGNAL(clicked()), this, SLOT(customImageSave()) );
	connect( conf->editNameCustomImagePushButton, SIGNAL(clicked()), this, SLOT(customImageEditName()) );
	connect( conf->deleteCustomImagePushButton, SIGNAL(clicked()), this, SLOT(customImageRemove()) );

	connect( conf->zipText, SIGNAL( textChanged( const QString & ) ), this, SLOT( zipTextChanged( const QString & ) ) );

	connect( conf->satelliteImagePushButton, SIGNAL( clicked() ), this, SLOT( satelliteImagePushButtonPressed() ) );

	connect( conf->xmlDataFileComboBox, SIGNAL( activated( int ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->zipText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->updateTimeCheckBox, SIGNAL( stateChanged( int ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->showTemperatureUnitCheckBox, SIGNAL( stateChanged( int ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->layoutComboBox, SIGNAL( activated( int ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->iconsProviderSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->iconsKDESelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->iconsCustomSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->iconsCustomKUrlRequester, SIGNAL( urlSelected( const KUrl & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->iconsCustomKUrlRequester, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->forecastSeparatorNoneSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->forecastSeparatorCustomSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->forecastSeparatorPlasmaSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->customImageKUrlRequester, SIGNAL( urlSelected( const KUrl & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->customImageKUrlRequester, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->backgroundFileComboBox, SIGNAL( activated( int ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->iconsScaleDoubleSpinBox, SIGNAL( valueChanged( double ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->feelsLikeText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->humidityText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->windText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->windRoseText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->invertWindRoseCheckBox, SIGNAL( stateChanged( int ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->windRoseScaleDoubleSpinBox, SIGNAL( valueChanged( double ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->rainText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->dewPointText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->visibilityText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->pressureText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->uvIndexText, SIGNAL( textChanged( const QString & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->defaultBackgroundSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->translucentBackgroundSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->noBackgroundSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->customColorBackgroundSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->customColorKColorButton, SIGNAL( changed( const QColor & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->defaultBackgroundSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->transparencySlider, SIGNAL( sliderMoved( int ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->fontComboBox, SIGNAL( activated( int ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->fontKColorButton, SIGNAL( changed( const QColor & ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->fontShadowCheckBox, SIGNAL( stateChanged( int ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->fontScaleDoubleSpinBox, SIGNAL( valueChanged( double ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->omitIconDescriptionCheckBox, SIGNAL( stateChanged( int ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->dayNamesSystemShortSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->dayNamesSystemLongSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->dayNamesSystemDateSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->dayNamesProviderSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->windLayoutRoseSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
	connect( conf->windLayoutProviderSelect, SIGNAL( clicked( bool ) ), this, SLOT( configAppearanceChanged() ) );
}

void Plasma_CWP::exportSettings()
{
	KUrl settingsfileurl = KFileDialog::getSaveUrl( KUrl(), "*.cwp" );
	if ( settingsfileurl.isEmpty() ) // no file chosen
	{
		qDebug() << "No settings file selected, aborting.";
		return;
	}
	QFile settingsfile( settingsfileurl.path() );
	if ( settingsfile.exists() )
	{
		qDebug() << "Settings file exists, overwrite?";
		if ( KMessageBox::questionYesNo( 0, i18n( "This will overwrite the CWP settings file. Continue?" ) ) == KMessageBox::No ) return;

		settingsfile.open( QIODevice::ReadOnly );
		if ( !settingsfile.isOpen() )
		{
			qDebug() << "Could not open settings file " << settingsfileurl;
			KMessageBox::information( 0, i18n( "Could not open CWP settings file %1." ).arg( settingsfileurl.path() ) );
			return;
		}
		settingsfile.close();
	}

	KConfig cf( settingsfileurl.path(), KConfig::SimpleConfig );
	KConfigGroup cg( &cf, "" );

	// copy and paste from configAccepted()
	if ( customImageList.size() == 1 && customImageNameList.size() == 1 && customImageNameList.at( 0 ) == "" )
	{
		customImageList.clear();
		customImageNameList.clear();
	}

	if ( conf->customImageKUrlRequester->url().url() != "" && customImageList.size() == 0 )
	{
		customImageList.prepend( conf->customImageKUrlRequester->url().url() );
		customImageNameList.prepend( "" );
	}

	if ( customImageCurrent < 0 ) customImageCurrent = 0;
	if ( customImageCurrent > customImageList.size() - 1 ) customImageCurrent = customImageList.size() - 1;

	cg.writeEntry( "cwpIdentifier", cwpIdentifier );
	cg.writeEntry( "xmlDataFile", conf->xmlDataFileComboBox->currentIndex() );
	cg.writeEntry( "zip", conf->zipText->text() );
	cg.writeEntry( "updateFrequency", updateFrequency.setNum( conf->freqChooser->value() ) );
	cg.writeEntry( "updateTime", conf->updateTimeCheckBox->isChecked() );
	cg.writeEntry( "showTemperatureUnit", conf->showTemperatureUnitCheckBox->isChecked() );

	cg.writeEntry( "provider_update_time_shown", provider_update_time_shown );

	cg.writeEntry( "layoutNumber", conf->layoutComboBox->currentIndex());
	cg.writeEntry( "icons", conf->iconsKDESelect->isChecked() ? 1 : ( conf->iconsCustomSelect->isChecked() ? 2 : 0 ) );
	cg.writeEntry( "iconsCustom", conf->iconsCustomKUrlRequester->url() );
	cg.writeEntry( "forecastSeparator", conf->forecastSeparatorCustomSelect->isChecked() ? 1 : ( conf->forecastSeparatorPlasmaSelect->isChecked() ? 2 : 0 ) );
	cg.writeEntry( "customImageList", customImageList );
	cg.writeEntry( "customImageNameList", customImageNameList );

	// remove pre-0.9.8 stuff
	cg.deleteEntry( "locationImage" );

	cg.writeEntry( "customImageCurrent", customImageCurrent );
	cg.writeEntry( "backgroundFile", conf->backgroundFileComboBox->currentIndex() );
	cg.writeEntry( "scaleIcons", conf->iconsScaleDoubleSpinBox->value() );
	cg.writeEntry( "feelsLike", conf->feelsLikeText->text() );
	cg.writeEntry( "humidity", conf->humidityText->text() );
	cg.writeEntry( "wind", conf->windText->text() );
	cg.writeEntry( "windRose", conf->windRoseText->text() );
	cg.writeEntry( "invertWindRose", conf->invertWindRoseCheckBox->isChecked() );
	cg.writeEntry( "windRoseScale", conf->windRoseScaleDoubleSpinBox->value() );
	cg.writeEntry( "rain", conf->rainText->text() );
	cg.writeEntry( "dewPoint", conf->dewPointText->text() );
	cg.writeEntry( "visibility", conf->visibilityText->text() );
	cg.writeEntry( "pressure", conf->pressureText->text() );
	cg.writeEntry( "uvIndex", conf->uvIndexText->text() );

	cg.writeEntry( "background", conf->translucentBackgroundSelect->isChecked() ? 1 : ( conf->noBackgroundSelect->isChecked() ? 2 : ( conf->customColorBackgroundSelect->isChecked() ? 3 : 0 ) ) );
	cg.writeEntry( "backgroundColor", conf->customColorKColorButton->color() );
	cg.writeEntry( "backgroundTransparency", conf->transparencySlider->value() );
	cg.writeEntry( "font", conf->fontComboBox->currentFont() );
	cg.writeEntry( "fontColor", conf->fontKColorButton->color() );
	cg.writeEntry( "fontShadow", conf->fontShadowCheckBox->isChecked() );
	cg.writeEntry( "scaleFont", conf->fontScaleDoubleSpinBox->value());

	cg.writeEntry( "omitIconDescription", conf->omitIconDescriptionCheckBox->isChecked() );

	if ( conf->dayNamesProviderSelect->isChecked() ) cg.writeEntry( "dayNamesSystem", 0 );
	else if ( conf->dayNamesSystemShortSelect->isChecked() ) cg.writeEntry( "dayNamesSystem", 1 );
	else if ( conf->dayNamesSystemLongSelect->isChecked() ) cg.writeEntry( "dayNamesSystem", 2 );
	else cg.writeEntry( "dayNamesSystem", 3 );

	cg.writeEntry( "windLayoutRose", conf->windLayoutRoseSelect->isChecked() );

	QList<int> index_list;
	QStringList name_list;
	QList<int> weather_provider_list;
	QStringList zip_list;
	int i;
	for ( i=0; i<preferred_location_list.size(); i++ )
	{
		index_list << preferred_location_list.at(i).index;
		name_list << preferred_location_list.at(i).name;
		weather_provider_list << preferred_location_list.at(i).weather_provider;
		zip_list << preferred_location_list.at(i).zip;
		cg.writeEntry( QString( "preferredLocationCustomImageList%1" ).arg( i ), preferred_location_list.at(i).custom_image_list );
		cg.writeEntry( QString( "preferredLocationCustomImageNameList%1" ).arg( i ), preferred_location_list.at(i).custom_image_name_list );
	}
	cg.writeEntry( "preferredLocationIndexList", index_list );
	cg.writeEntry( "preferredLocationNameList", name_list );
	cg.writeEntry( "preferredLocationWeatherProviderList", weather_provider_list );
	cg.writeEntry( "preferredLocationZipList", zip_list );

	// save size
	cg.writeEntry( "graphics_widget_size", graphics_widget->size() );
	cg.writeEntry( "custom_image_dialog_size", custom_image_dialog->size() );
	cg.writeEntry( "ProviderURL", provider_url );
	cg.writeEntry( "plasmoid_size", size() );
}

void Plasma_CWP::xmlDataFileSelected( int index )
{
	if ( conf )
	{
		if ( index < xml_data_file_list.size() && index >= 0 && xml_data_file_list.size() > 0 )
		{
			conf->exampleZipLabel->setText(
				i18n( "Find your city on the <a href=%1>weather provider's homepage</a> and copy the city identifier from the address line of your browser.<br>Example: %2",
					  xml_data_file_list.at(index).search_page, xml_data_file_list.at(index).example_zip ) );

			if ( conf->preferredLocationComboBox->currentIndex() >= 0 && conf->preferredLocationComboBox->currentIndex() < preferred_location_list.size() )
			{
				if ( index != preferred_location_list.at(conf->preferredLocationComboBox->currentIndex()).weather_provider )
				{
					if ( !conf->preferredLocationComboBox->currentText().endsWith( " *" ) )
					{
						conf->preferredLocationComboBox->setItemText( conf->preferredLocationComboBox->currentIndex(),
							preferred_location_list.at(conf->preferredLocationComboBox->currentIndex()).name + " *" );
					}
				}
				else
				{
					if ( conf->preferredLocationComboBox->currentText().endsWith( " *" ) &&
						index == preferred_location_list.at(conf->preferredLocationComboBox->currentIndex()).weather_provider &&
						conf->zipText->text() == preferred_location_list.at(conf->preferredLocationComboBox->currentIndex()).zip )
					{
						conf->preferredLocationComboBox->setItemText( conf->preferredLocationComboBox->currentIndex(),
							preferred_location_list.at(conf->preferredLocationComboBox->currentIndex()).name );
					}
				}
			}
		}
	}
}

void Plasma_CWP::zipTextChanged( const QString &text )
{
	if ( preferred_location_list.size() <= 0 || conf->preferredLocationComboBox->currentIndex() + 1 > preferred_location_list.size() ) return;

	if ( text != preferred_location_list.at(conf->preferredLocationComboBox->currentIndex()).zip )
	{
		if ( !conf->preferredLocationComboBox->currentText().endsWith( " *" ) )
		{
			conf->preferredLocationComboBox->setItemText( conf->preferredLocationComboBox->currentIndex(),
														  preferred_location_list.at(conf->preferredLocationComboBox->currentIndex()).name + " *" );
		}
	}
	else
	{
		if ( conf->preferredLocationComboBox->currentText().endsWith( " *" ) &&
			conf->xmlDataFileComboBox->currentIndex() == preferred_location_list.at(conf->preferredLocationComboBox->currentIndex()).weather_provider &&
			text == preferred_location_list.at(conf->preferredLocationComboBox->currentIndex()).zip )
		{
			conf->preferredLocationComboBox->setItemText( conf->preferredLocationComboBox->currentIndex(),
														  preferred_location_list.at(conf->preferredLocationComboBox->currentIndex()).name );
		}
	}
}

void Plasma_CWP::satelliteImagePushButtonPressed()
{
	if ( satellite_images_list.size() <= 0 || !conf ) return;

	QStringList satellite_images_list_tmp;

	int sel = 0;

	int i;
	for ( i=0; i<satellite_images_list.size(); i++ )
	{
		satellite_images_list_tmp.append( satellite_images_list.at(i).name );
		if ( satellite_images_list.at(i).url == conf->customImageKUrlRequester->url().url() ) sel = i;
	}

	if ( conf->customImageKUrlRequester->url().url() == "" ) sel = 0;

	bool ok;
	QString text = QInputDialog::getItem( conf, "Customizable Weather Plasmoid", i18n( "Select a satellite image:" ),
		satellite_images_list_tmp, sel, false /*editable*/, &ok );

	if ( ok && !text.isEmpty() )
	{
		for ( i=0; i<satellite_images_list_tmp.size(); i++ )
		{
			if ( text == satellite_images_list_tmp.at(i) )
			{
				if ( satellite_images_list.size() > 0 && i < satellite_images_list.size() && satellite_images_list.at(i).url != "" )
				{
					QList<KUrl> tmp_url_list;
					int j;
					for ( j=0; j<customImageList.size(); j++ )
					{
						tmp_url_list.append( KUrl( customImageList.at( j ) ) );
					}

					if ( xmlDataFile < xml_data_file_list.size() && xmlDataFile >= 0 && xml_data_file_list.size() > 0 && data_provider )
					{
						data_provider->set_config_values( updateFrequency, xml_data_file_list.at(xmlDataFile).file_name, zip, feelsLike,
							humidity, wind, tempType, tmp_url_list );
					}
					emit( reloadRequested() );

					conf->customImageKUrlRequester->setUrl( KUrl( satellite_images_list.at(i).url ) );

					emit( refreshRequested() );

					customImageUrlChanged( satellite_images_list.at(i).url );
				}
				break;
			}
		}
	}
}

void Plasma_CWP::preferredLocationSelected( int index )
{
	if ( preferred_location_list.size() <= 0 || index + 1 > preferred_location_list.size() ) return;

	if ( xml_data_file_list.size() <= 0 || preferred_location_list.at(index).weather_provider < 0 ||
		preferred_location_list.at(index).weather_provider + 1 > xml_data_file_list.size() )
	{
		return;
	}

	conf->xmlDataFileComboBox->setCurrentIndex( preferred_location_list.at(index).weather_provider );
	conf->zipText->setText( preferred_location_list.at(index).zip );
	customImageList = preferred_location_list.at(index).custom_image_list;
	customImageNameList = preferred_location_list.at(index).custom_image_name_list;

	conf->customImageListComboBox->clear();
	int i;
	for ( i=0; i<customImageList.size() && i<customImageNameList.size(); i++ )
	{
		if ( customImageNameList.at( i ) != "" ) conf->customImageListComboBox->addItem( customImageNameList.at( i ) );
	}

	if ( customImageCurrent >= 0 && customImageCurrent <= customImageNameList.size() - 1 )
	{
		conf->customImageListComboBox->setCurrentIndex( customImageCurrent );
	}

	if ( customImageCurrent >= 0 && customImageCurrent <= customImageList.size() - 1 )
	{
		conf->customImageKUrlRequester->setUrl( KUrl( customImageList.at( customImageCurrent ) ) );
	}
	else
	{
		if ( customImageList.size() > 0 ) conf->customImageKUrlRequester->setUrl( KUrl(  customImageList.at( 0 ) ) );
		else conf->customImageKUrlRequester->setUrl( KUrl( "" ) );
	}

	for ( i=0; i<conf->preferredLocationComboBox->count(); i++ )
	{
		conf->preferredLocationComboBox->setItemText( i, preferred_location_list.at(i).name );
	}
}

void Plasma_CWP::preferredLocationNew()
{
	if ( xml_data_file_list.size() <= 0 || conf->xmlDataFileComboBox->currentIndex() + 1 > xml_data_file_list.size() ) return;

	bool ok;
	QString text = QInputDialog::getText( conf, "Customizable Weather Plasmoid", i18n( "Name of preferred location:" ), QLineEdit::Normal, "", &ok );

	if ( ok && !text.isEmpty() )
	{
		PreferredLocation tmp_location;
		tmp_location.index = preferred_location_list.size();
		tmp_location.name = text;
		tmp_location.weather_provider = conf->xmlDataFileComboBox->currentIndex();
		tmp_location.zip = conf->zipText->text();
		tmp_location.custom_image_list = customImageList;
		tmp_location.custom_image_name_list = customImageNameList;

		preferred_location_list.append( tmp_location );

		conf->preferredLocationComboBox->addItem( text );

		conf->preferredLocationComboBox->setCurrentIndex( conf->preferredLocationComboBox->count() - 1 );

		int i;
		for ( i=0; i<conf->preferredLocationComboBox->count(); i++ )
		{
			conf->preferredLocationComboBox->setItemText( i, preferred_location_list.at(i).name );
		}
	}
}

void Plasma_CWP::preferredLocationSave()
{
	if ( xml_data_file_list.size() <= 0 || conf->xmlDataFileComboBox->currentIndex() + 1 > xml_data_file_list.size() )
	{
		return;
	}

	if ( conf->preferredLocationComboBox->currentIndex() < 0 ||
		conf->preferredLocationComboBox->currentIndex() + 1 > preferred_location_list.size() )
	{
		preferredLocationNew();
	}

	preferred_location_list[conf->preferredLocationComboBox->currentIndex()].weather_provider = conf->xmlDataFileComboBox->currentIndex();
	preferred_location_list[conf->preferredLocationComboBox->currentIndex()].zip = conf->zipText->text();
	preferred_location_list[conf->preferredLocationComboBox->currentIndex()].custom_image_list = customImageList;
	preferred_location_list[conf->preferredLocationComboBox->currentIndex()].custom_image_name_list = customImageNameList;

	int i;
	for ( i=0; i<conf->preferredLocationComboBox->count(); i++ )
	{
		conf->preferredLocationComboBox->setItemText( i, preferred_location_list.at(i).name );
	}
}

void Plasma_CWP::preferredLocationEditName()
{
	if ( conf->preferredLocationComboBox->currentIndex() < 0 ||
		conf->preferredLocationComboBox->currentIndex() + 1 > preferred_location_list.size() )
	{
		return;
	}

	bool ok;
	QString text = QInputDialog::getText( conf, "Customizable Weather Plasmoid", i18n( "Name of preferred location:" ), QLineEdit::Normal,
		preferred_location_list.at( conf->preferredLocationComboBox->currentIndex() ).name, &ok );

	if ( ok && !text.isEmpty() )
	{
		preferred_location_list[conf->preferredLocationComboBox->currentIndex()].name = text;
		if ( conf->preferredLocationComboBox->currentText().endsWith( " *" ) )
		{
			conf->preferredLocationComboBox->setItemText( conf->preferredLocationComboBox->currentIndex(), text + " *" );
		}
		else
		{
			conf->preferredLocationComboBox->setItemText( conf->preferredLocationComboBox->currentIndex(), text );
		}
	}
}

void Plasma_CWP::preferredLocationRemove()
{
	if ( conf->preferredLocationComboBox->currentIndex() < 0 ||
		conf->preferredLocationComboBox->currentIndex() + 1 > preferred_location_list.size() )
	{
		return;
	}

	preferred_location_list.removeAt( conf->preferredLocationComboBox->currentIndex() );

	int i;
	for ( i=0; i<preferred_location_list.size(); i++ ) preferred_location_list[i].index = i;

	conf->preferredLocationComboBox->removeItem( conf->preferredLocationComboBox->currentIndex() );

	preferredLocationSelected( conf->preferredLocationComboBox->currentIndex() );
}

void Plasma_CWP::currentPixmapChanged( int pixmap )
{
	customImageCurrent = pixmap;
	if ( customImageList.size() <= 0 )
	{
		customImageCurrent = -1;
	}
	else
	{
		if ( customImageCurrent < 0 ) customImageCurrent = 0;
		else if ( customImageCurrent > customImageList.size() - 1 ) customImageCurrent = customImageList.size() - 1;
	}

	if ( allow_config_saving )
	{
		KConfigGroup cg = config();
		cg.writeEntry( "customImageCurrent", customImageCurrent );
		emit configNeedsSaving();
	}
}

void Plasma_CWP::customImageUrlChanged( const QString &url )
{
	if ( customImageList.size() <= 0 || customImageList.size() != customImageNameList.size() ||
		conf->customImageListComboBox->currentIndex() + 1 > customImageList.size() ) return;

	if ( customImageList.size() == 1 && customImageNameList.size() == 1 && customImageNameList.at( 0 ) == "" )
	{
		QStringList customImageList_tmp = customImageList;

		customImageList_tmp[0] = conf->customImageKUrlRequester->url().url();

		int index = conf->preferredLocationComboBox->currentIndex();
		if ( index >= 0 && index < preferred_location_list.size() && preferred_location_list.size() > 0 &&
			index < xml_data_file_list.size() && xml_data_file_list.size() > 0 )
		{
			if ( customImageList_tmp != preferred_location_list.at( conf->preferredLocationComboBox->currentIndex() ).custom_image_list ||
				customImageNameList != preferred_location_list.at( conf->preferredLocationComboBox->currentIndex() ).custom_image_name_list )
			{
				conf->preferredLocationComboBox->setItemText( index, preferred_location_list.at( index ).name + " *" );
			}
			else
			{
				conf->preferredLocationComboBox->setItemText( index, preferred_location_list.at( index ).name );
			}
		}

		return;
	}

	if ( url != customImageList.at( conf->customImageListComboBox->currentIndex() ) )
	{
		if ( !conf->customImageListComboBox->currentText().endsWith( " *" ) )
		{
			conf->customImageListComboBox->setItemText( conf->customImageListComboBox->currentIndex(),
				customImageNameList.at( conf->customImageListComboBox->currentIndex() ) + " *" );
		}
	}
	else
	{
		if ( conf->customImageListComboBox->currentText().endsWith( " *" ) &&
			conf->customImageKUrlRequester->url().url() == customImageList.at( conf->customImageListComboBox->currentIndex() ) )
		{
			conf->customImageListComboBox->setItemText( conf->customImageListComboBox->currentIndex(),
				customImageNameList.at( conf->customImageListComboBox->currentIndex() ) );
		}
	}
}

void Plasma_CWP::customImageListSelected( int index )
{
	if ( customImageList.size() <= 0 || customImageList.size() != customImageNameList.size() ||
		index < 0 || index + 1 > customImageList.size() ) return;

	conf->customImageKUrlRequester->setUrl( KUrl( customImageList.at( index ) ) );

	if ( customImageNameList.size() != conf->customImageListComboBox->count() ) return;
	int i;
	for ( i=0; i<conf->customImageListComboBox->count(); i++ )
	{
		conf->customImageListComboBox->setItemText( i, customImageNameList.at( i ) );
	}
}

void Plasma_CWP::customImageNew()
{
	if ( conf->customImageKUrlRequester->url().url().isEmpty() ) return;

	bool ok;
	QString text = QInputDialog::getText( conf, "Customizable Weather Plasmoid", i18n( "Name of custom image:" ), QLineEdit::Normal, "", &ok );

	if ( ok && !text.isEmpty() )
	{
		if ( customImageList.size() == 1 && customImageNameList.size() == 1 )
		{
			if ( customImageNameList.at( 0 ) == "" )
			{
				customImageList.clear();
				customImageNameList.clear();
			}
		}

		customImageList.append( conf->customImageKUrlRequester->url().url() );
		customImageNameList.append( text );

		conf->customImageListComboBox->addItem( text );

		conf->customImageListComboBox->setCurrentIndex( conf->customImageListComboBox->count() - 1 );

		int i;
		for ( i=0; i<conf->customImageListComboBox->count(); i++ )
		{
			conf->customImageListComboBox->setItemText( i, customImageNameList.at(i) );
		}

		int index = conf->preferredLocationComboBox->currentIndex();
		if ( index >= 0 && index < preferred_location_list.size() && preferred_location_list.size() > 0 &&
			index < xml_data_file_list.size() && xml_data_file_list.size() > 0 )
		{
			if ( customImageList != preferred_location_list.at( conf->preferredLocationComboBox->currentIndex() ).custom_image_list ||
				customImageNameList != preferred_location_list.at( conf->preferredLocationComboBox->currentIndex() ).custom_image_name_list )
			{
				conf->preferredLocationComboBox->setItemText( index, preferred_location_list.at( index ).name + " *" );
			}
			else
			{
				conf->preferredLocationComboBox->setItemText( index, preferred_location_list.at( index ).name );
			}
		}
	}
}

void Plasma_CWP::customImageSave()
{
	if ( conf->customImageListComboBox->currentIndex() < 0 ||
		conf->customImageListComboBox->currentIndex() + 1 > customImageList.size() ||
		customImageList.size() != customImageNameList.size() )
	{
		return;
	}

	if ( conf->customImageListComboBox->currentIndex() < 0 ||
		conf->customImageListComboBox->currentIndex() + 1 > customImageList.size() )
	{
		customImageNew();
	}

	customImageList[conf->customImageListComboBox->currentIndex()] = conf->customImageKUrlRequester->url().url();

	if ( customImageNameList.size() != conf->customImageListComboBox->count() ) return;
	int i;
	for ( i=0; i<conf->customImageListComboBox->count(); i++ )
	{
		conf->customImageListComboBox->setItemText( i, customImageNameList.at( i ) );
	}

	int index = conf->preferredLocationComboBox->currentIndex();
	if ( index >= 0 && index < preferred_location_list.size() && preferred_location_list.size() > 0 &&
		index < xml_data_file_list.size() && xml_data_file_list.size() > 0 )
	{
		if ( customImageList != preferred_location_list.at( conf->preferredLocationComboBox->currentIndex() ).custom_image_list ||
			customImageNameList != preferred_location_list.at( conf->preferredLocationComboBox->currentIndex() ).custom_image_name_list )
		{
			conf->preferredLocationComboBox->setItemText( index, preferred_location_list.at( index ).name + " *" );
		}
		else
		{
			conf->preferredLocationComboBox->setItemText( index, preferred_location_list.at( index ).name );
		}
	}
}

void Plasma_CWP::customImageEditName()
{
	if ( conf->customImageListComboBox->currentIndex() < 0 ||
		conf->customImageListComboBox->currentIndex() + 1 > customImageList.size() ||
		customImageList.size() != customImageNameList.size() )
	{
		return;
	}

	bool ok;
	QString text = QInputDialog::getText( conf, "Customizable Weather Plasmoid", i18n( "Name of custom image:" ), QLineEdit::Normal,
		customImageNameList.at( conf->customImageListComboBox->currentIndex() ), &ok );

	if ( ok && !text.isEmpty() )
	{
		customImageNameList[conf->customImageListComboBox->currentIndex()] = text;
		if ( conf->customImageListComboBox->currentText().endsWith( " *" ) )
		{
			conf->customImageListComboBox->setItemText( conf->customImageListComboBox->currentIndex(), text + " *" );
		}
		else
		{
			conf->customImageListComboBox->setItemText( conf->customImageListComboBox->currentIndex(), text );
		}

		int index = conf->preferredLocationComboBox->currentIndex();
		if ( index >= 0 && index < preferred_location_list.size() && preferred_location_list.size() > 0 &&
			index < xml_data_file_list.size() && xml_data_file_list.size() > 0 )
		{
			if ( customImageList != preferred_location_list.at( conf->preferredLocationComboBox->currentIndex() ).custom_image_list ||
				customImageNameList != preferred_location_list.at( conf->preferredLocationComboBox->currentIndex() ).custom_image_name_list )
			{
				conf->preferredLocationComboBox->setItemText( index, preferred_location_list.at( index ).name + " *" );
			}
			else
			{
				conf->preferredLocationComboBox->setItemText( index, preferred_location_list.at( index ).name );
			}
		}
	}
}

void Plasma_CWP::customImageRemove()
{
	if ( conf->customImageListComboBox->currentIndex() < 0 ||
		conf->customImageListComboBox->currentIndex() + 1 > customImageList.size() ||
		customImageList.size() != customImageNameList.size() )
	{
		return;
	}

	customImageList.removeAt( conf->customImageListComboBox->currentIndex() );
	customImageNameList.removeAt( conf->customImageListComboBox->currentIndex() );

	conf->customImageListComboBox->removeItem( conf->customImageListComboBox->currentIndex() );

	customImageListSelected( conf->customImageListComboBox->currentIndex() );
	if ( customImageList.size() == 0 || customImageNameList.size() == 0 )
	{
		conf->customImageKUrlRequester->setUrl( KUrl( "" ) );
	}

	int index = conf->preferredLocationComboBox->currentIndex();
	if ( index >= 0 && index < preferred_location_list.size() && preferred_location_list.size() > 0 &&
		index < xml_data_file_list.size() && xml_data_file_list.size() > 0 )
	{
		if ( customImageList != preferred_location_list.at( conf->preferredLocationComboBox->currentIndex() ).custom_image_list ||
			customImageNameList != preferred_location_list.at( conf->preferredLocationComboBox->currentIndex() ).custom_image_name_list )
		{
			conf->preferredLocationComboBox->setItemText( index, preferred_location_list.at( index ).name + " *" );
		}
		else
		{
			conf->preferredLocationComboBox->setItemText( index, preferred_location_list.at( index ).name );
		}
	}
}

Qt::Orientations Plasma_CWP::expandingDirections() const
{
	// no use of additional space in any direction
	return 0;
}

void Plasma_CWP::createMenu()
{
	actions.clear();

	QAction *reload = new QAction( i18n("Reload Weather Data"), this );
	reload->setIcon( KIconLoader::global()->loadIcon( "view-refresh", KIconLoader::NoGroup,
		48, KIconLoader::DefaultState, QStringList(), 0L, false ) );
	actions.append( reload );
	if ( data_provider ) connect( reload, SIGNAL( triggered(bool) ), data_provider, SLOT( reloadData() ) );

	QAction *link = new QAction( i18n( "Link" ), this );
	link->setIcon( KIconLoader::global()->loadIcon( "internet-web-browser", KIconLoader::NoGroup,
		48, KIconLoader::DefaultState, QStringList(), 0L, false ) );
	actions.append( link );
	connect( link, SIGNAL( triggered(bool) ), this, SLOT( loadLink() ) );

	int i;
	for ( i=0; i<preferred_location_list.size() && i<25; i++ )
	{
		QAction *location = new QAction( preferred_location_list.at(i).name, this );
		actions.append( location );
		if      ( i ==  0 ) connect( location, SIGNAL( triggered(bool) ), this, SLOT( preferredLocationActionSelected0() ) );
		else if ( i ==  1 ) connect( location, SIGNAL( triggered(bool) ), this, SLOT( preferredLocationActionSelected1() ) );
		else if ( i ==  2 ) connect( location, SIGNAL( triggered(bool) ), this, SLOT( preferredLocationActionSelected2() ) );
		else if ( i ==  3 ) connect( location, SIGNAL( triggered(bool) ), this, SLOT( preferredLocationActionSelected3() ) );
		else if ( i ==  4 ) connect( location, SIGNAL( triggered(bool) ), this, SLOT( preferredLocationActionSelected4() ) );
		else if ( i ==  5 ) connect( location, SIGNAL( triggered(bool) ), this, SLOT( preferredLocationActionSelected5() ) );
		else if ( i ==  6 ) connect( location, SIGNAL( triggered(bool) ), this, SLOT( preferredLocationActionSelected6() ) );
		else if ( i ==  7 ) connect( location, SIGNAL( triggered(bool) ), this, SLOT( preferredLocationActionSelected7() ) );
		else if ( i ==  8 ) connect( location, SIGNAL( triggered(bool) ), this, SLOT( preferredLocationActionSelected8() ) );
		else if ( i ==  9 ) connect( location, SIGNAL( triggered(bool) ), this, SLOT( preferredLocationActionSelected9() ) );
		else if ( i == 10 ) connect( location, SIGNAL( triggered(bool) ), this, SLOT( preferredLocationActionSelected10() ) );
		else if ( i == 11 ) connect( location, SIGNAL( triggered(bool) ), this, SLOT( preferredLocationActionSelected11() ) );
		else if ( i == 12 ) connect( location, SIGNAL( triggered(bool) ), this, SLOT( preferredLocationActionSelected12() ) );
		else if ( i == 13 ) connect( location, SIGNAL( triggered(bool) ), this, SLOT( preferredLocationActionSelected13() ) );
		else if ( i == 14 ) connect( location, SIGNAL( triggered(bool) ), this, SLOT( preferredLocationActionSelected14() ) );
		else if ( i == 15 ) connect( location, SIGNAL( triggered(bool) ), this, SLOT( preferredLocationActionSelected15() ) );
		else if ( i == 16 ) connect( location, SIGNAL( triggered(bool) ), this, SLOT( preferredLocationActionSelected16() ) );
		else if ( i == 17 ) connect( location, SIGNAL( triggered(bool) ), this, SLOT( preferredLocationActionSelected17() ) );
		else if ( i == 18 ) connect( location, SIGNAL( triggered(bool) ), this, SLOT( preferredLocationActionSelected18() ) );
		else if ( i == 19 ) connect( location, SIGNAL( triggered(bool) ), this, SLOT( preferredLocationActionSelected19() ) );
		else if ( i == 20 ) connect( location, SIGNAL( triggered(bool) ), this, SLOT( preferredLocationActionSelected20() ) );
		else if ( i == 21 ) connect( location, SIGNAL( triggered(bool) ), this, SLOT( preferredLocationActionSelected21() ) );
		else if ( i == 22 ) connect( location, SIGNAL( triggered(bool) ), this, SLOT( preferredLocationActionSelected22() ) );
		else if ( i == 23 ) connect( location, SIGNAL( triggered(bool) ), this, SLOT( preferredLocationActionSelected23() ) );
		else if ( i == 24 ) connect( location, SIGNAL( triggered(bool) ), this, SLOT( preferredLocationActionSelected24() ) );
	}
}

QList<QAction*> Plasma_CWP::contextualActions()
{
	return actions;
}

void Plasma_CWP::loadLink()
{
	QDesktopServices::openUrl( KUrl( provider_url ) );
}

void Plasma_CWP::preferredLocationActionSelected( int index )
{
	if ( index < 0 && index + 1 > preferred_location_list.size() ) return;

	zip = preferred_location_list.at( index ).zip;
	xmlDataFile = preferred_location_list.at( index ).weather_provider;
	customImageList = preferred_location_list.at( index ).custom_image_list;
	customImageNameList = preferred_location_list.at( index ).custom_image_name_list;

	if ( allow_config_saving )
	{
		KConfigGroup cg = config();
		cg.writeEntry( "xmlDataFile", xmlDataFile );
		cg.writeEntry( "zip", zip );
		cg.writeEntry( "customImageList", customImageList );
		cg.writeEntry( "customImageNameList", customImageNameList );
		emit configNeedsSaving();
	}

	QList<KUrl> tmp_url_list;
	int i;
	for ( i=0; i<customImageList.size(); i++ )
	{
		tmp_url_list.append( KUrl( customImageList.at( i ) ) );
	}

	if ( data_provider )
	{
		data_provider->set_config_values( updateFrequency, xml_data_file_list.at(xmlDataFile).file_name, zip,
			feelsLike, humidity, wind, tempType, tmp_url_list );
		emit( reloadRequested() );
	}
}

double Plasma_CWP::getFontScale()
{
	double ret = 1.0;

	QSizeF current_size = ( size().height() >= 150 ? size() : graphics_widget->size() );
	double area = current_size.width() * current_size.height();

	if ( area < 150. * 150. ) return 1.0;

	switch( layoutNumber )
	{
		case DAYS_1_HORIZONTAL: // 1 horizontal
			ret = 1.00 * area / ( 400 * 100 );
			break;
		case DAYS_1_VERTICAL: // 1 vertical
			ret = 1.00 * area / ( 150 * 250 );
			break;
		case DAYS_3_HORIZONTAL: // 3 horizontal
			ret = 1.00 * area / ( 300 * 200 );
			break;
		case DAYS_3_VERTICAL: // 3 vertical
			ret = 1.00 * area / ( 200 * 300 );
			break;
		case DAYS_5_HORIZONTAL: // 5 horizontal
			ret = 1.00 * area / ( 500 * 200 );
			break;
		case DAYS_5_HORIZONTAL_COMPACT: // 5 horizontal (compact)
			ret = 1.00 * area / ( 400 * 200 );
			break;
		case DAYS_5_VERTICAL: // 5 vertical
			ret = 1.00 * area / ( 200 * 400 );
			break;
		case DAYS_7_HORIZONTAL: // 7 horizontal
			ret = 1.00 * area / ( 700 * 200 );
			break;
		case DAYS_7_HORIZONTAL_COMPACT: // 7 horizontal (compact)
			ret = 1.00 * area / ( 500 * 200 );
			break;
		case DAYS_7_VERTICAL: // 7 vertical
			ret = 1.00 * area / ( 200 * 500 );
			break;
		case DAYS_4_CUSTOM_IMAGE_HORIZONTAL: // 4 custom image (horizontal)
			ret = 0.90 * area / ( 400 * 200 );
			break;
		case DAYS_4_CUSTOM_IMAGE_VERTICAL: // 4 custom image (vertical)
			ret = 1.10 * area / ( 350 * 300 );
			break;
		case DAYS_7_CUSTOM_IMAGE: // 7 custom image
			ret = 1.20 * area / ( 450 * 300 );
			break;
		default:
			break;
	}

	return ret;
}

void Plasma_CWP::populateXmlDataFileList( const QDir &dir )
{
	if ( !dir.exists() ) return;

	kDebug() << "Looking for xml files inside " << dir.path();

	QStringList xml_candidates = dir.entryList( QStringList() << "*.xml", QDir::Files | QDir::Readable /*filters*/, QDir::Name /*sort*/ );

	kDebug() << "Found xml file candidates: " << xml_candidates;

	QString file_name_tmp;

	QString type, version, name, search_page, example_zip, unit;

	int index = 0;
	int i, j;
	for ( i=0; i<xml_candidates.size(); i++ )
	{
		file_name_tmp = dir.absoluteFilePath( xml_candidates.at(i) );

		type = "";
		version = "";
		name = "";
		search_page = "";
		example_zip = "";
		unit = "";

		QFile file_tmp( file_name_tmp );

		if ( file_tmp.exists() )
		{
			QDomDocument doc("weather_xml");
			doc.setContent(&file_tmp);
			file_tmp.close();

			QDomElement root = doc.documentElement();

			QDomNode n = root.firstChild();
			QDomNode n_old;

			while(!n.isNull())
			{
				QDomElement e = n.toElement();

				if(e.tagName() == "xml_file_version")
				{
					type = e.attribute("type", "");
					version = e.attribute("version", "");
					name = e.attribute("name", "");
					search_page = e.attribute("search_page", "");
					example_zip = e.attribute("example_zip", "");
					unit = e.attribute("unit", "");
				}

				n = n.nextSibling();
			}

			if ( type == "cwp" )
			{
				kDebug() << file_name_tmp << " seems to be a valid weather xml file!";

				DataFile xml_data_file_tmp;
				xml_data_file_tmp.index = index;
				xml_data_file_tmp.name = name;
				xml_data_file_tmp.version = version;
				xml_data_file_tmp.search_page = search_page;
				xml_data_file_tmp.example_zip = example_zip;
				xml_data_file_tmp.unit = unit;
				xml_data_file_tmp.file_name = file_name_tmp;

				// replace existing xml file, if it's the same provider and it's newer.
				// if it's the same provider, but older, ignore.
				// if it's a new provider, append.
				bool replaced = false;
				for ( j=0; j<xml_data_file_list.size(); j++ )
				{
					if ( xml_data_file_list.at( j ).name == xml_data_file_tmp.name &&
						xml_data_file_list.at( j ).unit == xml_data_file_tmp.unit )
					{
						if ( QString::compare( xml_data_file_list.at( j ).version, xml_data_file_tmp.version ) < 0 )
						{
							xml_data_file_list[j] = xml_data_file_tmp;
						}
						replaced = true;
						break;
					}
				}
				if ( !replaced ) xml_data_file_list.append( xml_data_file_tmp );

				index++;
			}
		}
	}
}

void Plasma_CWP::populateBackgroundFileList( const QDir &dir )
{
	if ( !dir.exists() ) return;

	kDebug() << "Looking for background files inside " << dir.path();

	QStringList background_candidates = dir.entryList(
		QStringList() << "*.png" << "*.gif" << "*.jpg", QDir::Files | QDir::Readable /*filters*/, QDir::Name /*sort*/ );

	kDebug() << "Found background candidates: " << background_candidates;

	QString file_name_tmp;

	QString type, name;

	int index = 0;
	int i;
	for ( i=0; i<background_candidates.size(); i++ )
	{
		file_name_tmp = dir.absoluteFilePath( background_candidates.at(i) );

		if ( QFile::exists( file_name_tmp ) )
		{
			if ( background_candidates.at(i).startsWith( "background_", Qt::CaseInsensitive ) )
			{
				kDebug() << file_name_tmp << " seems to be a valid background image file!";

				DataFile background_file_tmp;
				background_file_tmp.index = index;
				background_file_tmp.name = i18n( background_candidates.at(i).mid( 11, background_candidates.at(i).size()-4-11 ).replace( "_", " " ).toLatin1() );
				background_file_tmp.version = "";
				background_file_tmp.search_page = "";
				background_file_tmp.example_zip = "";
				background_file_tmp.unit = "";
				background_file_tmp.file_name = file_name_tmp;
				background_file_list.append( background_file_tmp );

				index++;
			}
		}
	}
}

void Plasma_CWP::populateSatelliteImagesList( const QDir &dir )
{
	if ( !dir.exists() ) return;

	kDebug() << "Looking for satellite image xml files inside " << dir.path();

	QStringList xml_candidates = dir.entryList( QStringList() << "*.xml", QDir::Files | QDir::Readable /*filters*/, QDir::Name /*sort*/ );

	kDebug() << "Found satellite image xml file candidates: " << xml_candidates;

	QString file_name_tmp;

	QString type;

	QList<int> index_list;
	QStringList name_list, version_list, url_list;

	int index = 0;
	int i, j;
	for ( i=0; i<xml_candidates.size(); i++ )
	{
		file_name_tmp = dir.absoluteFilePath( xml_candidates.at(i) );

		type = "";

		index_list.clear();
		version_list.clear();
		name_list.clear();
		url_list.clear();

		QFile file_tmp( file_name_tmp );

		if ( file_tmp.exists() )
		{
			QDomDocument doc( "satellite_images_xml" );
			doc.setContent( &file_tmp );
			file_tmp.close();

			QDomElement root = doc.documentElement();

			QDomNode n = root.firstChild();
			QDomNode n_old;

			while( !n.isNull() )
			{
				QDomElement e = n.toElement();

				if( e.tagName() == "xml_file_version" )
				{
					type = e.attribute("type", "");
				}

				if( e.tagName() == "image" )
				{
					index_list.append( index );
					index++;

					name_list.append( e.attribute( "name", "" ) );
					version_list.append( e.attribute( "version", "" ) );
					url_list.append( e.attribute( "url", "" ) );
				}

				n = n.nextSibling();
			}

			if ( type == "cwp_satellite_images" )
			{
				kDebug() << file_name_tmp << " seems to be a valid satellite images xml file!";

				for ( j=0; j<index_list.size(); j++ )
				{
					SatelliteImage satellite_image_tmp;
					satellite_image_tmp.index = index_list.at(j);
					satellite_image_tmp.name = i18n( name_list.at(j).toLatin1() );
					satellite_image_tmp.version = version_list.at(j);
					satellite_image_tmp.url = url_list.at(j);
					satellite_images_list.append( satellite_image_tmp );
				}
			}
		}
	}
}

void Plasma_CWP::customImageDialogHidden()
{
	if ( allow_config_saving )
	{
		KConfigGroup cg = config();
		cg.writeEntry( "custom_image_dialog_size", custom_image_dialog->size() );
		emit ( configNeedsSaving() );
	}
}

bool Plasma_CWP::isInsideLocationImage( const QPointF &p )
{
	if ( custom_image_full_rect.width() <= 0 || custom_image_full_rect.height() <= 0 ) return false;

	if ( p.x() >= custom_image_full_rect.x() &&
		p.x() < custom_image_full_rect.x() + custom_image_full_rect.width() &&
		p.y() >= custom_image_full_rect.y() &&
		p.y() < custom_image_full_rect.y() + custom_image_full_rect.height() ) return true;

	return false;
}

void Plasma_CWP::setLocationImageRect( const QRect &rect )
{
	custom_image_full_rect = rect;
}

bool Plasma_CWP::isInsideCurrentWeather( const QPointF &p )
{
	if ( current_weather_rect.width() <= 0 || current_weather_rect.height() <= 0 ) return false;

	if ( p.x() >= current_weather_rect.x() &&
		p.x() < current_weather_rect.x() + current_weather_rect.width() &&
		p.y() >= current_weather_rect.y() &&
		p.y() < current_weather_rect.y() + current_weather_rect.height() ) return true;

	return false;
}

void Plasma_CWP::setCurrentWeatherRect( const QRect &rect1, const QRect &rect2, const QRect &rect3,
										const QRect &rect4, const QRect &rect5 )
{
	QRect tmp_rect = rect1;
	tmp_rect = tmp_rect.united( rect2 );
	tmp_rect = tmp_rect.united( rect3 );
	tmp_rect = tmp_rect.united( rect4 );
	tmp_rect = tmp_rect.united( rect5 );

	current_weather_rect = tmp_rect;
}

bool Plasma_CWP::isInsideUpdateTime( const QPointF &p )
{
	if ( update_time_rect.width() <= 0 || update_time_rect.height() <= 0 ) return false;

	if ( p.x() >= update_time_rect.x() &&
		p.x() < update_time_rect.x() + update_time_rect.width() &&
		p.y() >= update_time_rect.y() &&
		p.y() < update_time_rect.y() + update_time_rect.height() ) return true;

	return false;
}

void Plasma_CWP::setUpdateTimeRect( const QRect &rect )
{
	update_time_rect = rect;
}

void Plasma_CWP::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
	if ( event->button() == Qt::LeftButton )
	{
		if ( isInsideUpdateTime( event->pos() ) )
		{
			if ( !extended_weather_information_dialog->isHidden() )
			{
				extended_weather_information_dialog->hide();
			}
			else
			{
				if ( !custom_image_dialog->isHidden() )
				{
					custom_image_dialog->hide();
				}
				else
				{
					provider_update_time_shown = provider_update_time_shown ? false : true;
					if ( allow_config_saving )
					{
						KConfigGroup cg = config();
						cg.writeEntry( "provider_update_time_shown", provider_update_time_shown );
						emit ( configNeedsSaving() );
					}
					updateGraphicsWidget();
				}
			}
		}
		else if ( isInsideCurrentWeather( event->pos() ) )
		{
			if ( !extended_weather_information_dialog->isHidden() )
			{
				extended_weather_information_dialog->hide();
			}
			else
			{
				if ( !custom_image_dialog->isHidden() )
				{
					custom_image_dialog->hide();
				}
				else
				{
					extended_weather_information_dialog->move( popupPosition( extended_weather_information_dialog->size() ) );
					extended_weather_information_dialog->show();
				}
			}
		}
		else
		{
			if ( !extended_weather_information_dialog->isHidden() )
			{
				extended_weather_information_dialog->hide();
			}
			else
			{
				if ( !custom_image_dialog->isHidden() )
				{
					custom_image_dialog->hide();
				}
				else if ( isInsideLocationImage( event->pos() ) )
				{
					custom_image_dialog->move( popupPosition( custom_image_dialog->size() ) );
					custom_image_dialog->show();
				}
			}
		}
	}
}

#include "plasma-cwp.moc"
