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

#include <QDomDocument>
#include <QTemporaryFile>
#include <QTimer>
#include <QTextCodec>

#include <KDebug>

#include <KShell>
#include <KIO/Job>

#include "plasma-cwp-data-provider.h"

Data_Provider::Data_Provider( QObject *parent, const QString &path, const uint identifier ) :
	QObject( parent ), local_data_path( path ), local_data_identifier( identifier )
{
	locale = "C";
	encoding = "";

	data_update_time = "";
	data_provider_update_time = "";

	xmlFile = "";

	int i;
	url_follow_command_list.clear();
	for ( i=0; i<8; i++ )
	{
		url_follow_command_list.push_back( NULL );
	}
	data_command_list.clear();
	for ( i=0; i<DATA_COMMAND_NUMBER; i++ )
	{
		data_command_list.push_back( NULL );
	}
	url_follow_command_data_string.clear();
	url_follow_command_command.clear();
	for ( i=0; i<8; i++ )
	{
		url_follow_command_data_string.push_back( "" );
		url_follow_command_command.push_back( "" );
	}
	data_command_data_string.clear();
	data_command_command.clear();
	data_command_data_result.clear();
	for ( i=0; i<DATA_COMMAND_NUMBER; i++ )
	{
		data_command_data_string.push_back( "" );
		data_command_command.push_back( "" );
		data_command_data_result.push_back( "" );
	}

	data_job_list.clear();
	for ( i=0; i<8; i++ ) data_job_list.push_back( NULL );
	url_follow_job_list.clear();
	for ( i=0; i<8; i++ ) url_follow_job_list.push_back( NULL );
	image_job_list.clear();
	icon_job_list.clear();
	for ( i=0; i<8; i++ ) icon_job_list.push_back( NULL );

	data_job_success_list.clear();
	for ( i=0; i<8; i++ ) data_job_success_list.push_back( true );
	url_follow_job_success_list.clear();
	for ( i=0; i<8; i++ ) url_follow_job_success_list.push_back( true );
	image_job_success_list.clear();
	icon_job_success_list.clear();
	for ( i=0; i<8; i++ ) icon_job_success_list.push_back( true );

	urlFollowCommandRunning = -1;
	dataCommandRunning = -1;

	last_download_succeeded = false;

	connect( this, SIGNAL( signalLoadLocalData() ), this, SLOT( loadLocalData() ) );

	// load weather data saved inside <path> using <identifier>
	emit( signalLoadLocalData() );

	// Get updates whenever networking status was changed
	connect( Solid::Networking::notifier(), SIGNAL( statusChanged( Solid::Networking::Status ) ), this, SLOT( solidNetworkingStatusChanged( Solid::Networking::Status ) ) );
}

Data_Provider::~Data_Provider()
{
	// save weather data saved inside <path> using <identifier>
	saveLocalData();

	int i;
	for ( i=0; i<8; i++ )
	{
		if ( url_follow_command_list[i] ) url_follow_command_list[i]->kill();
		delete url_follow_command_list[i];
	}
	for ( i=0; i<DATA_COMMAND_NUMBER; i++ )
	{
		if ( data_command_list[i] ) data_command_list[i]->kill();
		delete data_command_list[i];
	}
}

void Data_Provider::solidNetworkingStatusChanged( Solid::Networking::Status status )
{
	if ( status == Solid::Networking::Connected && !last_download_succeeded )
	{
		reloadData();
	}
}

void Data_Provider::loadLocalData()
{
	if ( local_data_path == "" || local_data_identifier == 0 )
	{
		return;
	}

	KConfig cf( local_data_path + QString( "cwp_local_data_%1.cfg" ).arg( local_data_identifier ), KConfig::SimpleConfig );
	KConfigGroup cfg( &cf, "" );

	data_update_time = cfg.readEntry( "data_update_time", "" );
	data_provider_update_time = cfg.readEntry( "data_provider_update_time", "" );

	data_location_location = cfg.readEntry( "data_location_location", "" );
	data_location_country = cfg.readEntry( "data_location_country", "" );

	data_sun_sunrise = cfg.readEntry( "data_sun_sunrise", "" );
	data_sun_sunset = cfg.readEntry( "data_sun_sunset", "" );

	data_current_temperature = cfg.readEntry( "data_current_temperature", "" );
	data_current_temperature_felt = cfg.readEntry( "data_current_temperature_felt", "" );
	data_current_wind_code = cfg.readEntry( "data_current_wind_code", "" );
	data_current_wind_speed = cfg.readEntry( "data_current_wind_speed", "" );
	data_current_wind = cfg.readEntry( "data_current_wind", "" );
	data_current_humidity = cfg.readEntry( "data_current_humidity", "" );
	data_current_icon_code = cfg.readEntry( "data_current_icon_code", -1 );

	data_current_icon_text = cfg.readEntry( "data_current_icon_text", "" );

	data_current_rain = cfg.readEntry( "data_current_rain", "" );
	data_current_dew_point = cfg.readEntry( "data_current_dew_point", "" );
	data_current_visibility = cfg.readEntry( "data_current_visibility", "" );
	data_current_pressure = cfg.readEntry( "data_current_pressure", "" );
	data_current_uv_index = cfg.readEntry( "data_current_uv_index", "" );

	data_day_name[0] = cfg.readEntry( "data_day_name[0]", "" );
	data_day_temperature_high[0] = cfg.readEntry( "data_day_temperature_high[0]", "" );
	data_day_temperature_low[0] = cfg.readEntry( "data_day_temperature_low[0]", "" );
	data_day_icon_code[0] = cfg.readEntry( "data_day_icon_code[0]", -1 );
	data_day_icon_text[0] = cfg.readEntry( "data_day_icon_text[0]", "" );

	data_day_name[1] = cfg.readEntry( "data_day_name[1]", "" );
	data_day_temperature_high[1] = cfg.readEntry( "data_day_temperature_high[1]", "" );
	data_day_temperature_low[1] = cfg.readEntry( "data_day_temperature_low[1]", "" );
	data_day_icon_code[1] = cfg.readEntry( "data_day_icon_code[1]", -1 );
	data_day_icon_text[1] = cfg.readEntry( "data_day_icon_text[1]", "" );

	data_day_name[2] = cfg.readEntry( "data_day_name[2]", "" );
	data_day_temperature_high[2] = cfg.readEntry( "data_day_temperature_high[2]", "" );
	data_day_temperature_low[2] = cfg.readEntry( "data_day_temperature_low[2]", "" );
	data_day_icon_code[2] = cfg.readEntry( "data_day_icon_code[2]", -1 );
	data_day_icon_text[2] = cfg.readEntry( "data_day_icon_text[2]", "" );

	data_day_name[3] = cfg.readEntry( "data_day_name[3]", "" );
	data_day_temperature_high[3] = cfg.readEntry( "data_day_temperature_high[3]", "" );
	data_day_temperature_low[3] = cfg.readEntry( "data_day_temperature_low[3]", "" );
	data_day_icon_code[3] = cfg.readEntry( "data_day_icon_code[3]", -1 );
	data_day_icon_text[3] = cfg.readEntry( "data_day_icon_text[3]", "" );

	data_day_name[4] = cfg.readEntry( "data_day_name[4]", "" );
	data_day_temperature_high[4] = cfg.readEntry( "data_day_temperature_high[4]", "" );
	data_day_temperature_low[4] = cfg.readEntry( "data_day_temperature_low[4]", "" );
	data_day_icon_code[4] = cfg.readEntry( "data_day_icon_code[4]", -1 );
	data_day_icon_text[4] = cfg.readEntry( "data_day_icon_text[4]", "" );

	data_day_name[5] = cfg.readEntry( "data_day_name[5]", "" );
	data_day_temperature_high[5] = cfg.readEntry( "data_day_temperature_high[5]", "" );
	data_day_temperature_low[5] = cfg.readEntry( "data_day_temperature_low[5]", "" );
	data_day_icon_code[5] = cfg.readEntry( "data_day_icon_code[5]", -1 );
	data_day_icon_text[5] = cfg.readEntry( "data_day_icon_text[5]", "" );

	data_day_name[6] = cfg.readEntry( "data_day_name[6]", "" );
	data_day_temperature_high[6] = cfg.readEntry( "data_day_temperature_high[6]", "" );
	data_day_temperature_low[6] = cfg.readEntry( "data_day_temperature_low[6]", "" );
	data_day_icon_code[6] = cfg.readEntry( "data_day_icon_code[6]", -1 );
	data_day_icon_text[6] = cfg.readEntry( "data_day_icon_text[6]", "" );

	// png must be used, as images (icons nearly always) contain transparent parts. jpg doesn't support transparency.
	int i;
	data_custom_image_list.clear();
	for ( i=0; i<100; i++ ) // random maximum location image number
	{
		QFile file( local_data_path + QString( "cwp_custom_image_%1_%2.img" ).arg( i ).arg( local_data_identifier ) );
		if ( !file.open(QIODevice::ReadOnly) ) break;
		data_custom_image_list.push_back( file.readAll() );
	}
	data_current_icon = QImage( local_data_path + QString( "cwp_icon_current_%1.png" ).arg( local_data_identifier ) );
	data_day_icon[0] = QImage( local_data_path + QString( "cwp_icon_0_%1.png" ).arg( local_data_identifier ) );
	data_day_icon[1] = QImage( local_data_path + QString( "cwp_icon_1_%1.png" ).arg( local_data_identifier ) );
	data_day_icon[2] = QImage( local_data_path + QString( "cwp_icon_2_%1.png" ).arg( local_data_identifier ) );
	data_day_icon[3] = QImage( local_data_path + QString( "cwp_icon_3_%1.png" ).arg( local_data_identifier ) );
	data_day_icon[4] = QImage( local_data_path + QString( "cwp_icon_4_%1.png" ).arg( local_data_identifier ) );
	data_day_icon[5] = QImage( local_data_path + QString( "cwp_icon_5_%1.png" ).arg( local_data_identifier ) );
	data_day_icon[6] = QImage( local_data_path + QString( "cwp_icon_6_%1.png" ).arg( local_data_identifier ) );

	if ( data_update_time != "" ) emit( data_fetched() );
}

void Data_Provider::saveLocalData()
{
	if ( local_data_path == "" || local_data_identifier == 0 )
	{
		return;
	}

	KConfig cf( local_data_path + QString( "cwp_local_data_%1.cfg" ).arg( local_data_identifier ), KConfig::SimpleConfig );
	KConfigGroup cfg( &cf, "" );

	cfg.writeEntry( "data_update_time", data_update_time );
	cfg.writeEntry( "data_provider_update_time", data_provider_update_time );

	cfg.writeEntry( "data_location_location", data_location_location );
	cfg.writeEntry( "data_location_country", data_location_country );

	cfg.writeEntry( "data_sun_sunrise", data_sun_sunrise );
	cfg.writeEntry( "data_sun_sunset", data_sun_sunset );

	cfg.writeEntry( "data_current_temperature", data_current_temperature );
	cfg.writeEntry( "data_current_temperature_felt", data_current_temperature_felt );
	cfg.writeEntry( "data_current_wind_code", data_current_wind_code );
	cfg.writeEntry( "data_current_wind_speed", data_current_wind_speed );
	cfg.writeEntry( "data_current_wind", data_current_wind );
	cfg.writeEntry( "data_current_humidity", data_current_humidity );
	cfg.writeEntry( "data_current_icon_code", data_current_icon_code );

	cfg.writeEntry( "data_current_icon_text", data_current_icon_text );

	cfg.writeEntry( "data_current_rain", data_current_rain );
	cfg.writeEntry( "data_current_dew_point", data_current_dew_point );
	cfg.writeEntry( "data_current_visibility", data_current_visibility );
	cfg.writeEntry( "data_current_pressure", data_current_pressure );
	cfg.writeEntry( "data_current_uv_index", data_current_uv_index );

	cfg.writeEntry( "data_day_name[0]", data_day_name[0] );
	cfg.writeEntry( "data_day_temperature_high[0]", data_day_temperature_high[0] );
	cfg.writeEntry( "data_day_temperature_low[0]", data_day_temperature_low[0] );
	cfg.writeEntry( "data_day_icon_code[0]", data_day_icon_code[0] );
	cfg.writeEntry( "data_day_icon_text[0]", data_day_icon_text[0] );

	cfg.writeEntry( "data_day_name[1]", data_day_name[1] );
	cfg.writeEntry( "data_day_temperature_high[1]", data_day_temperature_high[1] );
	cfg.writeEntry( "data_day_temperature_low[1]", data_day_temperature_low[1] );
	cfg.writeEntry( "data_day_icon_code[1]", data_day_icon_code[1] );
	cfg.writeEntry( "data_day_icon_text[1]", data_day_icon_text[1] );

	cfg.writeEntry( "data_day_name[2]", data_day_name[2] );
	cfg.writeEntry( "data_day_temperature_high[2]", data_day_temperature_high[2] );
	cfg.writeEntry( "data_day_temperature_low[2]", data_day_temperature_low[2] );
	cfg.writeEntry( "data_day_icon_code[2]", data_day_icon_code[2] );
	cfg.writeEntry( "data_day_icon_text[2]", data_day_icon_text[2] );

	cfg.writeEntry( "data_day_name[3]", data_day_name[3] );
	cfg.writeEntry( "data_day_temperature_high[3]", data_day_temperature_high[3] );
	cfg.writeEntry( "data_day_temperature_low[3]", data_day_temperature_low[3] );
	cfg.writeEntry( "data_day_icon_code[3]", data_day_icon_code[3] );
	cfg.writeEntry( "data_day_icon_text[3]", data_day_icon_text[3] );

	cfg.writeEntry( "data_day_name[4]", data_day_name[4] );
	cfg.writeEntry( "data_day_temperature_high[4]", data_day_temperature_high[4] );
	cfg.writeEntry( "data_day_temperature_low[4]", data_day_temperature_low[4] );
	cfg.writeEntry( "data_day_icon_code[4]", data_day_icon_code[4] );
	cfg.writeEntry( "data_day_icon_text[4]", data_day_icon_text[4] );

	cfg.writeEntry( "data_day_name[5]", data_day_name[5] );
	cfg.writeEntry( "data_day_temperature_high[5]", data_day_temperature_high[5] );
	cfg.writeEntry( "data_day_temperature_low[5]", data_day_temperature_low[5] );
	cfg.writeEntry( "data_day_icon_code[5]", data_day_icon_code[5] );
	cfg.writeEntry( "data_day_icon_text[5]", data_day_icon_text[5] );

	cfg.writeEntry( "data_day_name[6]", data_day_name[6] );
	cfg.writeEntry( "data_day_temperature_high[6]", data_day_temperature_high[6] );
	cfg.writeEntry( "data_day_temperature_low[6]", data_day_temperature_low[6] );
	cfg.writeEntry( "data_day_icon_code[6]", data_day_icon_code[6] );
	cfg.writeEntry( "data_day_icon_text[6]", data_day_icon_text[6] );

	// png must be used, as images (icons nearly always) contain transparent parts. jpg doesn't support transparency.
	int i;
	for ( i=0; i<data_custom_image_list.size(); i++ )
	{
		QFile file( local_data_path + QString( "cwp_custom_image_%1_%2.img" ).arg( i ).arg( local_data_identifier ) );
		file.open( QIODevice::WriteOnly );
		file.write( data_custom_image_list.at( i ) );
	}
	data_current_icon.save( local_data_path + QString( "cwp_icon_current_%1.png" ).arg( local_data_identifier ) );
	data_day_icon[0].save( local_data_path + QString( "cwp_icon_0_%1.png" ).arg( local_data_identifier ) );
	data_day_icon[1].save( local_data_path + QString( "cwp_icon_1_%1.png" ).arg( local_data_identifier ) );
	data_day_icon[2].save( local_data_path + QString( "cwp_icon_2_%1.png" ).arg( local_data_identifier ) );
	data_day_icon[3].save( local_data_path + QString( "cwp_icon_3_%1.png" ).arg( local_data_identifier ) );
	data_day_icon[4].save( local_data_path + QString( "cwp_icon_4_%1.png" ).arg( local_data_identifier ) );
	data_day_icon[5].save( local_data_path + QString( "cwp_icon_5_%1.png" ).arg( local_data_identifier ) );
	data_day_icon[6].save( local_data_path + QString( "cwp_icon_6_%1.png" ).arg( local_data_identifier ) );
}

void Data_Provider::get_weather_values( QString &data_location_location_tmp, QString &data_location_country_tmp, QString &data_sun_sunrise_tmp,
	QString &data_sun_sunset_tmp, QString &data_current_temperature_tmp, QString &data_current_temperature_felt_tmp,
	QString &data_current_wind_code_tmp, QString &data_current_wind_speed_tmp, QString &data_current_wind_tmp,
	QString &data_current_humidity_tmp, QString &data_current_dew_point_tmp,
	QString &data_current_visibility_tmp, QString &data_current_pressure_tmp, QString &data_current_uv_index_tmp,
	int &data_current_icon_code_tmp, QImage &data_current_icon_tmp,
	QString &data_current_icon_text_tmp, QString &data_current_rain_tmp, QString *data_day_name_tmp,
	QString *data_day_temperature_high_tmp, QString *data_day_temperature_low_tmp,
	int *data_day_icon_code_tmp, QImage *data_day_icon_tmp, QString *data_day_icon_text_tmp, QString &data_update_time_tmp,
	QString &data_provider_update_time_tmp, QString &tempType_tmp, QList<QByteArray> &data_custom_image_list_tmp,
	bool &last_download_succeeded_tmp, QString &provider_url_tmp )
{
	data_update_time_tmp = data_update_time;
	data_provider_update_time_tmp = data_provider_update_time;
	data_location_location_tmp = data_location_location;
	data_location_country_tmp = data_location_country;
	data_sun_sunrise_tmp = data_sun_sunrise;
	data_sun_sunset_tmp = data_sun_sunset;
	data_current_temperature_tmp = data_current_temperature;
	data_current_temperature_felt_tmp = data_current_temperature_felt;
	data_current_wind_code_tmp = data_current_wind_code;
	data_current_wind_speed_tmp = data_current_wind_speed;
	data_current_wind_tmp = data_current_wind;
	data_current_humidity_tmp = data_current_humidity;
	data_current_icon_code_tmp = data_current_icon_code;
	data_current_icon_tmp = data_current_icon;
	data_current_icon_text_tmp = data_current_icon_text;
	data_current_rain_tmp = data_current_rain;
	data_current_dew_point_tmp = data_current_dew_point;
	data_current_visibility_tmp = data_current_visibility;
	data_current_pressure_tmp = data_current_pressure;
	data_current_uv_index_tmp = data_current_uv_index;
	tempType_tmp = tempType;
	data_custom_image_list_tmp = data_custom_image_list;

	int i;
	for( i=0; i<7; i++ )
	{
		data_day_name_tmp[i] = data_day_name[i];
		data_day_temperature_high_tmp[i] = data_day_temperature_high[i];
		data_day_temperature_low_tmp[i] = data_day_temperature_low[i];
		data_day_icon_code_tmp[i] = data_day_icon_code[i];
		data_day_icon_tmp[i] = data_day_icon[i];
		data_day_icon_text_tmp[i] = data_day_icon_text[i];
	}

	last_download_succeeded_tmp = last_download_succeeded;

	if ( data_current_temperature_url_xml == "urlc" )
	{
		if ( urlc_follow_xml != "" ) provider_url_tmp = urlc_new;
		else provider_url_tmp = urlc_prefix_xml + zip + urlc_suffix_xml;
	}
	else if ( data_current_temperature_url_xml == "url1" )
	{
		if ( url1_follow_xml != "" ) provider_url_tmp = url1_new;
		else provider_url_tmp = url1_prefix_xml + zip + url1_suffix_xml;
	}
	else if ( data_current_temperature_url_xml == "url2" )
	{
		if ( url2_follow_xml != "" ) provider_url_tmp = url2_new;
		else provider_url_tmp = url2_prefix_xml + zip + url2_suffix_xml;
	}
	else if ( data_current_temperature_url_xml == "url3" )
	{
		if ( url3_follow_xml != "" ) provider_url_tmp = url3_new;
		else provider_url_tmp = url3_prefix_xml + zip + url3_suffix_xml;
	}
	else if ( data_current_temperature_url_xml == "url4" )
	{
		if ( url4_follow_xml != "" ) provider_url_tmp = url4_new;
		else provider_url_tmp = url4_prefix_xml + zip + url4_suffix_xml;
	}
	else if ( data_current_temperature_url_xml == "url5" )
	{
		if ( url5_follow_xml != "" ) provider_url_tmp = url5_new;
		else provider_url_tmp = url5_prefix_xml + zip + url5_suffix_xml;
	}
	else if ( data_current_temperature_url_xml == "url6" )
	{
		if ( url6_follow_xml != "" ) provider_url_tmp = url6_new;
		else provider_url_tmp = url6_prefix_xml + zip + url6_suffix_xml;
	}
	else if ( data_current_temperature_url_xml == "url7" )
	{
		if ( url7_follow_xml != "" ) provider_url_tmp = url7_new;
		else provider_url_tmp = url7_prefix_xml + zip + url7_suffix_xml;
	}
}

void Data_Provider::set_config_values( const QString &updateFrequency_tmp, const QString &xmlFile_tmp, const QString &zip_tmp, const QString &feelsLike_tmp,
	const QString &humidity_tmp, const QString &wind_tmp, const QString &tempType_tmp, const QList<KUrl> &customImageList_tmp )
{
	updateFrequency = updateFrequency_tmp;
	xmlFile = xmlFile_tmp;
	zip = zip_tmp;
	feelsLike = feelsLike_tmp;
	humidity = humidity_tmp;
	wind = wind_tmp;
	tempType = tempType_tmp;
	customImageList = customImageList_tmp;
}

void Data_Provider::reloadData()
{
	int i;
	for ( i=0; i<data_job_list.size(); i++ )
	{
		if ( data_job_list[i] )
		{
			disconnect( data_job_list[i], SIGNAL( data( KIO::Job *, const QByteArray & ) ), this, SLOT( dataDownloadedData( KIO::Job *, const QByteArray & ) ) );
			disconnect( data_job_list[i], SIGNAL( result( KJob * ) ), this, SLOT( dataDownloadFinished( KJob * ) ) );
			delete data_job_list[i];
			data_job_list[i] = NULL;
		}
	}
	for ( i=0; i<url_follow_job_list.size(); i++ )
	{
		if ( data_job_list[i] )
		{
			disconnect( url_follow_job_list[i], SIGNAL( data( KIO::Job *, const QByteArray & ) ), this, SLOT( urlFollowDownloadedData( KIO::Job *, const QByteArray & ) ) );
			disconnect( url_follow_job_list[i], SIGNAL( result( KJob * ) ), this, SLOT( urlFollowDownloadFinished( KJob * ) ) );
			delete url_follow_job_list[i];
			url_follow_job_list[i] = NULL;
		}
	}
	for ( i=0; i<image_job_list.size(); i++ )
	{
		if ( image_job_list[i] )
		{
			disconnect( image_job_list[i], SIGNAL( data( KIO::Job *, const QByteArray & ) ), this, SLOT( imageDownloadedData( KIO::Job *, const QByteArray & ) ) );
			disconnect( image_job_list[i], SIGNAL( result( KJob * ) ), this, SLOT( imageDownloadFinished( KJob * ) ) );
			delete image_job_list[i];
			image_job_list[i] = NULL;
		}
	}
	for ( i=0; i<icon_job_list.size(); i++ )
	{
		if ( icon_job_list[i] )
		{
			disconnect( icon_job_list[i], SIGNAL( data( KIO::Job *, const QByteArray & ) ), this, SLOT( iconDownloadedData( KIO::Job *, const QByteArray & ) ) );
			disconnect( icon_job_list[i], SIGNAL( result( KJob * ) ), this, SLOT( iconDownloadFinished( KJob * ) ) );
			delete icon_job_list[i];
			icon_job_list[i] = NULL;
		}
	}

	for ( i=0; i<8; i++ )
	{
		if ( url_follow_command_list[i] )
		{
			url_follow_command_list[i]->kill();
		}
	}
	for ( i=0; i<DATA_COMMAND_NUMBER; i++ )
	{
		if ( data_command_list[i] )
		{
			data_command_list[i]->kill();
		}
	}

	for ( i=0; i<WEATHER_CODE_TRANSFORM_SIZE; i++ )
	{
		data_icon_transform_in_xml[i] = "";
		data_icon_transform_out_xml[i] = "";
	}

	for ( i=0; i<WIND_TRANSFORM_SIZE; i++ )
	{
		data_wind_transform_in_xml[i] = "";
		data_wind_transform_out_xml[i] = "";
	}

	QDomDocument doc( "weather_xml" );
	QFile file( xmlFile );

	//read the data into the XML parser and close the file
	doc.setContent( &file );
	file.close();

	QDomElement root = doc.documentElement();

	QDomNode n = root.firstChild();
	QDomNode n_old;

	while( !n.isNull() )
	{
		QDomElement e = n.toElement();

		if( e.tagName() == "locale_settings" )
		{
			locale_xml = e.attribute( "locale", "" );
			encoding_xml = e.attribute( "encoding", "" );
		}

		if( e.tagName() == "urlc" )
		{
			urlc_prefix_xml = e.attribute( "urlc_prefix", "" );
			urlc_suffix_xml = e.attribute( "urlc_suffix", "" );
			urlc_follow_xml = e.attribute( "urlc_follow", "" );
		}

		if( e.tagName() == "url1" )
		{
			url1_prefix_xml = e.attribute( "url1_prefix", "" );
			url1_suffix_xml = e.attribute( "url1_suffix", "" );
			url1_follow_xml = e.attribute( "url1_follow", "" );
		}

		if( e.tagName() == "url2" )
		{
			url2_prefix_xml = e.attribute( "url2_prefix", "" );
			url2_suffix_xml = e.attribute( "url2_suffix", "" );
			url2_follow_xml = e.attribute( "url2_follow", "" );
		}

		if( e.tagName() == "url3" )
		{
			url3_prefix_xml = e.attribute( "url3_prefix", "" );
			url3_suffix_xml = e.attribute( "url3_suffix", "" );
			url3_follow_xml = e.attribute( "url3_follow", "" );
		}

		if( e.tagName() == "url4" )
		{
			url4_prefix_xml = e.attribute( "url4_prefix", "" );
			url4_suffix_xml = e.attribute( "url4_suffix", "" );
			url4_follow_xml = e.attribute( "url4_follow", "" );
		}

		if( e.tagName() == "url5" )
		{
			url5_prefix_xml = e.attribute( "url5_prefix", "" );
			url5_suffix_xml = e.attribute( "url5_suffix", "" );
			url5_follow_xml = e.attribute( "url5_follow", "" );
		}

		if( e.tagName() == "url6" )
		{
			url6_prefix_xml = e.attribute( "url6_prefix", "" );
			url6_suffix_xml = e.attribute( "url6_suffix", "" );
			url6_follow_xml = e.attribute( "url6_follow", "" );
		}

		if( e.tagName() == "url7" )
		{
			url7_prefix_xml = e.attribute( "url7_prefix", "" );
			url7_suffix_xml = e.attribute( "url7_suffix", "" );
			url7_follow_xml = e.attribute( "url7_follow", "" );
		}

		if( e.tagName() == "data_location" )
		{
			data_location_url_xml = e.attribute( "url", "" );
			data_location_location_xml = e.attribute( "location", "" );
			data_location_country_xml = e.attribute( "country", "" );
		}

		if( e.tagName() == "data_sun" )
		{
			data_sun_url_xml = e.attribute( "url", "" );
			data_sun_sunrise_xml = e.attribute( "sunrise", "" );
			data_sun_sunset_xml = e.attribute( "sunset", "" );
		}

		if( e.tagName() == "data_current_temperature" )
		{
			data_current_temperature_url_xml = e.attribute( "url", "" );
			data_provider_update_time_xml = e.attribute( "update_time", "" );
			data_current_temperature_xml = e.attribute( "temperature", "" );
			data_current_temperature_felt_xml = e.attribute( "temperature_felt", "" );
		}

		if( e.tagName() == "data_current_wind" )
		{
			data_current_wind_url_xml = e.attribute( "url", "" );
			data_current_wind_code_xml = e.attribute( "wind_code", "" );
			data_current_wind_speed_xml = e.attribute( "wind_speed", "" );
			data_current_wind_xml = e.attribute( "wind", "" );
		}

		if( e.tagName() == "data_current_icon" )
		{
			data_current_icon_url_xml = e.attribute( "url", "" );
			data_current_icon_code_xml = e.attribute( "icon_code", "" );
			data_current_icon_xml = e.attribute( "icon", "" );
			data_current_icon_text_xml = e.attribute( "icon_text", "" );
		}

		if( e.tagName() == "data_current_additional" )
		{
			data_current_additional_url_xml = e.attribute( "url", "" );
			data_current_humidity_xml = e.attribute( "humidity", "" );
			data_current_rain_xml = e.attribute( "rain", "" );
			data_current_dew_point_xml = e.attribute( "dew_point", "" );
			data_current_visibility_xml = e.attribute( "visibility", "" );
			data_current_pressure_xml = e.attribute( "pressure", "" );
			data_current_uv_index_xml = e.attribute( "uv_index", "" );
		}

		if( e.tagName() == "data_day1" )
		{
			data_day_url_xml[0] = e.attribute( "url", "" );
			data_day_name_xml[0] = e.attribute( "name", "" );
			data_day_temperature_high_xml[0] = e.attribute( "temperature_high", "" );
			data_day_temperature_low_xml[0] = e.attribute( "temperature_low", "" );
			data_day_icon_code_xml[0] = e.attribute( "icon_code", "" );
			data_day_icon_xml[0] = e.attribute( "icon", "" );
			data_day_icon_text_xml[0] = e.attribute( "icon_text", "" );
		}

		if( e.tagName() == "data_day2" )
		{
			data_day_url_xml[1] = e.attribute( "url", "" );
			data_day_name_xml[1] = e.attribute( "name", "" );
			data_day_temperature_high_xml[1] = e.attribute( "temperature_high", "" );
			data_day_temperature_low_xml[1] = e.attribute( "temperature_low", "" );
			data_day_icon_code_xml[1] = e.attribute( "icon_code", "" );
			data_day_icon_xml[1] = e.attribute( "icon", "" );
			data_day_icon_text_xml[1] = e.attribute( "icon_text", "" );
		}

		if( e.tagName() == "data_day3" )
		{
			data_day_url_xml[2] = e.attribute( "url", "" );
			data_day_name_xml[2] = e.attribute( "name", "" );
			data_day_temperature_high_xml[2] = e.attribute( "temperature_high", "" );
			data_day_temperature_low_xml[2] = e.attribute( "temperature_low", "" );
			data_day_icon_code_xml[2] = e.attribute( "icon_code", "" );
			data_day_icon_xml[2] = e.attribute( "icon", "" );
			data_day_icon_text_xml[2] = e.attribute( "icon_text", "" );
		}

		if( e.tagName() == "data_day4" )
		{
			data_day_url_xml[3] = e.attribute( "url", "" );
			data_day_name_xml[3] = e.attribute( "name", "" );
			data_day_temperature_high_xml[3] = e.attribute( "temperature_high", "" );
			data_day_temperature_low_xml[3] = e.attribute( "temperature_low", "" );
			data_day_icon_code_xml[3] = e.attribute( "icon_code", "" );
			data_day_icon_xml[3] = e.attribute( "icon", "" );
			data_day_icon_text_xml[3] = e.attribute( "icon_text", "" );
		}

		if( e.tagName() == "data_day5" )
		{
			data_day_url_xml[4] = e.attribute( "url", "" );
			data_day_name_xml[4] = e.attribute( "name", "" );
			data_day_temperature_high_xml[4] = e.attribute( "temperature_high", "" );
			data_day_temperature_low_xml[4] = e.attribute( "temperature_low", "" );
			data_day_icon_code_xml[4] = e.attribute( "icon_code", "" );
			data_day_icon_xml[4] = e.attribute( "icon", "" );
			data_day_icon_text_xml[4] = e.attribute( "icon_text", "" );
		}

		if( e.tagName() == "data_day6" )
		{
			data_day_url_xml[5] = e.attribute( "url", "" );
			data_day_name_xml[5] = e.attribute( "name", "" );
			data_day_temperature_high_xml[5] = e.attribute( "temperature_high", "" );
			data_day_temperature_low_xml[5] = e.attribute( "temperature_low", "" );
			data_day_icon_code_xml[5] = e.attribute( "icon_code", "" );
			data_day_icon_xml[5] = e.attribute( "icon", "" );
			data_day_icon_text_xml[5] = e.attribute( "icon_text", "" );
		}

		if( e.tagName() == "data_day7" )
		{
			data_day_url_xml[6] = e.attribute( "url", "" );
			data_day_name_xml[6] = e.attribute( "name", "" );
			data_day_temperature_high_xml[6] = e.attribute( "temperature_high", "" );
			data_day_temperature_low_xml[6] = e.attribute( "temperature_low", "" );
			data_day_icon_code_xml[6] = e.attribute( "icon_code", "" );
			data_day_icon_xml[6] = e.attribute( "icon", "" );
			data_day_icon_text_xml[6] = e.attribute( "icon_text", "" );
		}

		if( e.tagName() == "icon_transform" )
		{
			QString transform_number;
			int n;
			for ( n=0; n<WEATHER_CODE_TRANSFORM_SIZE; n++ )
			{
				transform_number.setNum( n+1 );
				data_icon_transform_in_xml[n] = e.attribute(QString( "i" ) + transform_number, "" );
				data_icon_transform_out_xml[n] = e.attribute(QString( "o" ) + transform_number, "" );
			}
		}

		if( e.tagName() == "wind_transform" )
		{
			QString transform_number;
			int n;
			for ( n=0; n<WIND_TRANSFORM_SIZE; n++ )
			{
				transform_number.setNum( n+1 );
				data_wind_transform_in_xml[n] = e.attribute(QString( "i" ) + transform_number, "" );
				data_wind_transform_out_xml[n] = e.attribute(QString( "o" ) + transform_number, "" );
			}
		}

		n = n.nextSibling();
	}

	data_download_data_list.clear();
	for ( i=0; i<8; i++ )
	{
		data_download_data_list.push_back( QByteArray() );
	}
	data_image_list.clear();
	data_icon_list.clear();
	for ( i=0; i<8; i++ )
	{
		data_icon_list.push_back( QByteArray() );
	}

	startDownloadData();
}

void Data_Provider::startDownloadData()
{
	url_followed = false;
	url_follow_command_run = false;
	data_command_run = false;

	int i;
	data_job_success_list.clear();
	for ( i=0; i<8; i++ ) data_job_success_list.push_back( true );
	url_follow_job_success_list.clear();
	for ( i=0; i<8; i++ ) url_follow_job_success_list.push_back( true );
	image_job_success_list.clear();
	icon_job_success_list.clear();
	for ( i=0; i<8; i++ ) icon_job_success_list.push_back( true );

	if ( urlc_prefix_xml != "" )
	{
		data_job_success_list[0] = false;
		KIO::Job *jobc = KIO::get( KUrl( urlc_prefix_xml + zip + urlc_suffix_xml ), KIO::Reload, KIO::HideProgressInfo );
		connect( jobc, SIGNAL( data( KIO::Job *, const QByteArray & ) ), this, SLOT( dataDownloadedData( KIO::Job *, const QByteArray & ) ) );
		connect( jobc, SIGNAL( result( KJob * ) ), this, SLOT( dataDownloadFinished( KJob * ) ) );
		data_job_list[0] = jobc;
		jobc->start();
	}

	if ( url1_prefix_xml != "" )
	{
		data_job_success_list[1] = false;
		KIO::Job *job1 = KIO::get( KUrl( url1_prefix_xml + zip + url1_suffix_xml ), KIO::Reload, KIO::HideProgressInfo );
		connect( job1, SIGNAL( data( KIO::Job *, const QByteArray & ) ), this, SLOT( dataDownloadedData( KIO::Job *, const QByteArray & ) ) );
		connect( job1, SIGNAL( result( KJob * ) ), this, SLOT( dataDownloadFinished( KJob * ) ) );
		data_job_list[1] = job1;
		job1->start();
	}

	if ( url2_prefix_xml != "" )
	{
		data_job_success_list[2] = false;
		KIO::Job *job2 = KIO::get( KUrl( url2_prefix_xml + zip + url2_suffix_xml ), KIO::Reload, KIO::HideProgressInfo );
		connect( job2, SIGNAL( data( KIO::Job *, const QByteArray & ) ), this, SLOT( dataDownloadedData( KIO::Job *, const QByteArray & ) ) );
		connect( job2, SIGNAL( result( KJob * ) ), this, SLOT( dataDownloadFinished( KJob * ) ) );
		data_job_list[2] = job2;
		job2->start();
	}

	if ( url3_prefix_xml != "" )
	{
		data_job_success_list[3] = false;
		KIO::Job *job3 = KIO::get( KUrl( url3_prefix_xml + zip + url3_suffix_xml ), KIO::Reload, KIO::HideProgressInfo );
		connect( job3, SIGNAL( data( KIO::Job *, const QByteArray & ) ), this, SLOT( dataDownloadedData( KIO::Job *, const QByteArray & ) ) );
		connect( job3, SIGNAL( result( KJob * ) ), this, SLOT( dataDownloadFinished( KJob * ) ) );
		data_job_list[3] = job3;
		job3->start();
	}

	if ( url4_prefix_xml != "" )
	{
		data_job_success_list[4] = false;
		KIO::Job *job4 = KIO::get( KUrl( url4_prefix_xml + zip + url4_suffix_xml ), KIO::Reload, KIO::HideProgressInfo );
		connect( job4, SIGNAL( data( KIO::Job *, const QByteArray & ) ), this, SLOT( dataDownloadedData( KIO::Job *, const QByteArray & ) ) );
		connect( job4, SIGNAL( result( KJob * ) ), this, SLOT( dataDownloadFinished( KJob * ) ) );
		data_job_list[4] = job4;
		job4->start();
	}

	if ( url5_prefix_xml != "" )
	{
		data_job_success_list[5] = false;
		KIO::Job *job5 = KIO::get( KUrl( url5_prefix_xml + zip + url5_suffix_xml ), KIO::Reload, KIO::HideProgressInfo );
		connect( job5, SIGNAL( data( KIO::Job *, const QByteArray & ) ), this, SLOT( dataDownloadedData( KIO::Job *, const QByteArray & ) ) );
		connect( job5, SIGNAL( result( KJob * ) ), this, SLOT( dataDownloadFinished( KJob * ) ) );
		data_job_list[5] = job5;
		job5->start();
	}

	if ( url6_prefix_xml != "" )
	{
		data_job_success_list[6] = false;
		KIO::Job *job6 = KIO::get( KUrl( url6_prefix_xml + zip + url6_suffix_xml ), KIO::Reload, KIO::HideProgressInfo );
		connect( job6, SIGNAL( data( KIO::Job *, const QByteArray & ) ), this, SLOT( dataDownloadedData( KIO::Job *, const QByteArray & ) ) );
		connect( job6, SIGNAL( result( KJob * ) ), this, SLOT( dataDownloadFinished( KJob * ) ) );
		data_job_list[6] = job6;
		job6->start();
	}

	if ( url7_prefix_xml != "" )
	{
		data_job_success_list[7] = false;
		KIO::Job *job7 = KIO::get( KUrl( url7_prefix_xml + zip + url7_suffix_xml ), KIO::Reload, KIO::HideProgressInfo );
		connect( job7, SIGNAL( data( KIO::Job *, const QByteArray & ) ), this, SLOT( dataDownloadedData( KIO::Job *, const QByteArray & ) ) );
		connect( job7, SIGNAL( result( KJob * ) ), this, SLOT( dataDownloadFinished( KJob * ) ) );
		data_job_list[7] = job7;
		job7->start();
	}
}

void Data_Provider::parseData()
{
	if ( !url_followed )
	{
		if ( urlc_follow_xml == "" &&
			url1_follow_xml == "" &&
			url2_follow_xml == "" &&
			url3_follow_xml == "" &&
			url4_follow_xml == "" &&
			url5_follow_xml == "" &&
			url6_follow_xml == "" &&
			url7_follow_xml == "" )
		{
			url_followed = true;
		}
		else
		{
			if ( !url_follow_command_run )
			{
				urlFollowCommandAppend( urlc_follow_xml, "urlc", 0 );
				urlFollowCommandAppend( url1_follow_xml, "url1", 1 );
				urlFollowCommandAppend( url2_follow_xml, "url2", 2 );
				urlFollowCommandAppend( url3_follow_xml, "url3", 3 );
				urlFollowCommandAppend( url4_follow_xml, "url4", 4 );
				urlFollowCommandAppend( url5_follow_xml, "url5", 5 );
				urlFollowCommandAppend( url6_follow_xml, "url6", 6 );
				urlFollowCommandAppend( url7_follow_xml, "url7", 7 );

				urlFollowCommandStartExecution();

				return;
			}
			else
			{
				if ( urlc_follow_xml != "" )
				{
					url_follow_job_success_list[0] = false;
					data_download_data_list[0] = QByteArray();
					KIO::Job *jobc = KIO::get( KUrl( urlc_new ), KIO::Reload, KIO::HideProgressInfo );
					connect( jobc, SIGNAL( data( KIO::Job *, const QByteArray & ) ), this, SLOT( urlFollowDownloadedData( KIO::Job *, const QByteArray & ) ) );
					connect( jobc, SIGNAL( result( KJob * ) ), this, SLOT( urlFollowDownloadFinished( KJob * ) ) );
					url_follow_job_list[0] = jobc;
					jobc->start();
				}

				if ( url1_follow_xml != "" )
				{
					url_follow_job_success_list[1] = false;
					data_download_data_list[1] = QByteArray();
					KIO::Job *job1 = KIO::get( KUrl( url1_new ), KIO::Reload, KIO::HideProgressInfo );
					connect( job1, SIGNAL( data( KIO::Job *, const QByteArray & ) ), this, SLOT( urlFollowDownloadedData( KIO::Job *, const QByteArray & ) ) );
					connect( job1, SIGNAL( result( KJob * ) ), this, SLOT( urlFollowDownloadFinished( KJob * ) ) );
					url_follow_job_list[1] = job1;
					job1->start();
				}

				if ( url2_follow_xml != "" )
				{
					url_follow_job_success_list[2] = false;
					data_download_data_list[2] = QByteArray();
					KIO::Job *job2 = KIO::get( KUrl( url2_new ), KIO::Reload, KIO::HideProgressInfo );
					connect( job2, SIGNAL( data( KIO::Job *, const QByteArray & ) ), this, SLOT( urlFollowDownloadedData( KIO::Job *, const QByteArray & ) ) );
					connect( job2, SIGNAL( result( KJob * ) ), this, SLOT( urlFollowDownloadFinished( KJob * ) ) );
					url_follow_job_list[2] = job2;
					job2->start();
				}

				if ( url3_follow_xml != "" )
				{
					url_follow_job_success_list[3] = false;
					data_download_data_list[3] = QByteArray();
					KIO::Job *job3 = KIO::get( KUrl( url3_new ), KIO::Reload, KIO::HideProgressInfo );
					connect( job3, SIGNAL( data( KIO::Job *, const QByteArray & ) ), this, SLOT( urlFollowDownloadedData( KIO::Job *, const QByteArray & ) ) );
					connect( job3, SIGNAL( result( KJob * ) ), this, SLOT( urlFollowDownloadFinished( KJob * ) ) );
					url_follow_job_list[3] = job3;
					job3->start();
				}

				if ( url4_follow_xml != "" )
				{
					url_follow_job_success_list[4] = false;
					data_download_data_list[4] = QByteArray();
					KIO::Job *job4 = KIO::get( KUrl( url4_new ), KIO::Reload, KIO::HideProgressInfo );
					connect( job4, SIGNAL( data( KIO::Job *, const QByteArray & ) ), this, SLOT( urlFollowDownloadedData( KIO::Job *, const QByteArray & ) ) );
					connect( job4, SIGNAL( result( KJob * ) ), this, SLOT( urlFollowDownloadFinished( KJob * ) ) );
					url_follow_job_list[4] = job4;
					job4->start();
				}

				if ( url5_follow_xml != "" )
				{
					url_follow_job_success_list[5] = false;
					data_download_data_list[5] = QByteArray();
					KIO::Job *job5 = KIO::get( KUrl( url5_new ), KIO::Reload, KIO::HideProgressInfo );
					connect( job5, SIGNAL( data( KIO::Job *, const QByteArray & ) ), this, SLOT( urlFollowDownloadedData( KIO::Job *, const QByteArray & ) ) );
					connect( job5, SIGNAL( result( KJob * ) ), this, SLOT( urlFollowDownloadFinished( KJob * ) ) );
					url_follow_job_list[5] = job5;
					job5->start();
				}

				if ( url6_follow_xml != "" )
				{
					url_follow_job_success_list[6] = false;
					data_download_data_list[6] = QByteArray();
					KIO::Job *job6 = KIO::get( KUrl( url6_new ), KIO::Reload, KIO::HideProgressInfo );
					connect( job6, SIGNAL( data( KIO::Job *, const QByteArray & ) ), this, SLOT( urlFollowDownloadedData( KIO::Job *, const QByteArray & ) ) );
					connect( job6, SIGNAL( result( KJob * ) ), this, SLOT( urlFollowDownloadFinished( KJob * ) ) );
					url_follow_job_list[6] = job6;
					job6->start();
				}

				if ( url7_follow_xml != "" )
				{
					url_follow_job_success_list[7] = false;
					data_download_data_list[7] = QByteArray();
					KIO::Job *job7 = KIO::get( KUrl( url7_new ), KIO::Reload, KIO::HideProgressInfo );
					connect( job7, SIGNAL( data( KIO::Job *, const QByteArray & ) ), this, SLOT( urlFollowDownloadedData( KIO::Job *, const QByteArray & ) ) );
					connect( job7, SIGNAL( result( KJob * ) ), this, SLOT( urlFollowDownloadFinished( KJob * ) ) );
					url_follow_job_list[7] = job7;
					job7->start();
				}

				return;
			}
		}
	}

	// was download of data successful?
	int i;
	last_download_succeeded = true;
	for ( i=0; i<8; i++ )
	{
		if ( !data_job_success_list[i] ) last_download_succeeded = false;
	}
	for ( i=0; i<8; i++ )
	{
		if ( !url_follow_job_success_list[i] ) last_download_succeeded = false;
	}
	if ( !last_download_succeeded )
	{
		qDebug() << "Data download failed!" << endl;
		return;
	}

	if ( !data_command_run )
	{
		// run commands from xml file on weather files
		locale = locale_xml;
		encoding = encoding_xml;
		qDebug() << "setting locale to " << locale << " and encoding to " << encoding;

		dataCommandAppend( data_location_location_xml, data_location_url_xml, 0 );
		dataCommandAppend( data_location_country_xml, data_location_url_xml, 1 );
		dataCommandAppend( data_sun_sunrise_xml, data_sun_url_xml, 2 );
		dataCommandAppend( data_sun_sunset_xml, data_sun_url_xml, 3 );

		dataCommandAppend( data_provider_update_time_xml, data_current_temperature_url_xml, 4 );

		dataCommandAppend( data_current_temperature_xml, data_current_temperature_url_xml, 5 );
		dataCommandAppend( data_current_temperature_felt_xml, data_current_temperature_url_xml, 6 );
		dataCommandAppend( data_current_wind_code_xml, data_current_wind_url_xml, 7 );
		dataCommandAppend( data_current_wind_speed_xml, data_current_wind_url_xml, 8 );
		dataCommandAppend( data_current_wind_xml, data_current_wind_url_xml, 9 );

		dataCommandAppend( data_current_icon_code_xml, data_current_icon_url_xml, 10 );
		dataCommandAppend( data_current_icon_xml, data_current_icon_url_xml, 11 );
		dataCommandAppend( data_current_icon_text_xml, data_current_icon_url_xml, 12 );
		dataCommandAppend( data_current_humidity_xml, data_current_additional_url_xml, 13 );
		dataCommandAppend( data_current_rain_xml, data_current_additional_url_xml, 14 );
		dataCommandAppend( data_current_dew_point_xml, data_current_additional_url_xml, 15 );
		dataCommandAppend( data_current_visibility_xml, data_current_additional_url_xml, 16 );
		dataCommandAppend( data_current_pressure_xml, data_current_additional_url_xml, 17 );
		dataCommandAppend( data_current_uv_index_xml, data_current_additional_url_xml, 18 );

		for(i=0; i<7; i++)
		{
			dataCommandAppend( data_day_name_xml[i], data_day_url_xml[i], 19 + i * 6 + 0 );
			dataCommandAppend( data_day_temperature_high_xml[i], data_day_url_xml[i], 19 + i * 6 + 1 );
			dataCommandAppend( data_day_temperature_low_xml[i], data_day_url_xml[i], 19 + i * 6 + 2 );
			dataCommandAppend( data_day_icon_code_xml[i], data_day_url_xml[i], 19 + i * 6 + 3 );
			dataCommandAppend( data_day_icon_xml[i], data_day_url_xml[i], 19 + i * 6 + 4 );
			dataCommandAppend( data_day_icon_text_xml[i], data_day_url_xml[i], 19 + i * 6 + 5 );
		}

		// download custom images
		data_image_list.clear();
		for ( i=0; i<customImageList.size(); i++ )
		{
			data_image_list.push_back( QByteArray() );
		}
		image_job_list.clear();
		for ( i=0; i<customImageList.size(); i++ )
		{
			image_job_list.push_back( NULL );
		}
		image_job_success_list.clear();
		for ( i=0; i<customImageList.size(); i++ )
		{
			image_job_success_list.push_back( false );
		}

		KIO::Job *job_image = NULL;
		bool job_started = false;
		for ( i=0; i<customImageList.size(); i++ )
		{
			if ( customImageList.at( i ).isLocalFile() )
			{
				QFile file( customImageList.at( i ).toLocalFile() );
				if ( file.open( QIODevice::ReadOnly ) )
				{
					data_image_list[i] = file.readAll();
					image_job_success_list[i] = true;
				}
			}
			else
			{
				job_started = true;

				image_job_success_list.push_back( false );
				job_image = KIO::get( KUrl( customImageList.at( i ) ), KIO::Reload, KIO::HideProgressInfo );
				connect( job_image, SIGNAL( data( KIO::Job *, const QByteArray & ) ), this, SLOT( imageDownloadedData( KIO::Job *, const QByteArray & ) ) );
				connect( job_image, SIGNAL( result( KJob * ) ), this, SLOT( imageDownloadFinished( KJob * ) ) );
				image_job_list[i] = job_image;
				job_image->start();
			}
		}
		if ( !job_started )
		{
			data_custom_image_list.clear();
			for ( i=0; i<image_job_success_list.size(); i++ )
			{
				if ( image_job_success_list[i] )
				{
					data_custom_image_list.push_back( data_image_list[i] );
				}
			}
			data_image_list.clear();
		}

		dataCommandStartExecution();

		return;
	}

	data_location_location = dataFromIdentifier( 0 );
	qDebug() << "data_location_location: " << data_location_location;
	data_location_country = dataFromIdentifier( 1 );
	qDebug() << "data_location_country: " << data_location_country;
	data_sun_sunrise = dataFromIdentifier( 2 );
	qDebug() << "data_sun_sunrise: " << data_sun_sunrise;
	data_sun_sunset = dataFromIdentifier( 3 );
	qDebug() << "data_sun_sunset: " << data_sun_sunset;
	data_provider_update_time = dataFromIdentifier( 4 );
	qDebug() << "data_provider_update_time: " << data_provider_update_time;
	data_current_temperature = dataFromIdentifier( 5 );
	qDebug() << "data_current_temperature: " << data_current_temperature;
	data_current_temperature_felt = dataFromIdentifier( 6 );
	qDebug() << "data_current_temperature_felt: " << data_current_temperature_felt;
	data_current_wind_code = dataFromIdentifier( 7 );
	int j;
	for ( j=0; j<WIND_TRANSFORM_SIZE; j++ )
	{
		if ( QString::compare( dataFromIdentifier( 7 ), data_wind_transform_in_xml[j], Qt::CaseInsensitive ) == 0 )
			data_current_wind_code = data_wind_transform_out_xml[j];
	}
	qDebug() << "data_current_wind_code: " << data_current_wind_code;
	data_current_wind_speed = dataFromIdentifier( 8 );
	qDebug() << "data_current_wind_speed: " << data_current_wind_speed;
	data_current_wind = dataFromIdentifier( 9 );
	qDebug() << "data_current_wind: " << data_current_wind;

	qDebug() << "data_current_icon_code (orig): " << dataFromIdentifier( 10 );
	data_current_icon_code = dataFromIdentifier( 10 ).toInt();
	for ( j=0; j<WEATHER_CODE_TRANSFORM_SIZE; j++ )
	{
		if ( QString::compare( dataFromIdentifier( 10 ), data_icon_transform_in_xml[j], Qt::CaseInsensitive ) == 0 )
		{
			data_current_icon_code = data_icon_transform_out_xml[j].toInt();
		}
	}
	qDebug() << "data_current_icon_code: " << data_current_icon_code;
	data_current_icon_text = dataFromIdentifier( 12 );
	qDebug() << "data_current_icon_text: " << data_current_icon_text;
	data_current_humidity = dataFromIdentifier( 13 );
	qDebug() << "data_current_humidity: " << data_current_humidity;
	data_current_rain = dataFromIdentifier( 14 );
	qDebug() << "data_current_rain: " << data_current_rain;
	data_current_dew_point = dataFromIdentifier( 15 );
	qDebug() << "data_current_dew_point: " << data_current_dew_point;
	data_current_visibility = dataFromIdentifier( 16 );
	qDebug() << "data_current_visibility: " << data_current_visibility;
	data_current_pressure = dataFromIdentifier( 17 );
	qDebug() << "data_current_pressure: " << data_current_pressure;
	data_current_uv_index = dataFromIdentifier( 18 );
	qDebug() << "data_current_uv_index: " << data_current_uv_index;

	for( i=0; i<7; i++ )
	{
		data_day_name[i] = dataFromIdentifier( 19 + i * 6 + 0 );
		qDebug() << "data_day_name[" << i << "]: " << data_day_name[i];
		data_day_temperature_high[i] = dataFromIdentifier( 19 + i * 6 + 1 );
		qDebug() << "data_day_temperature_high[" << i << "]: " << data_day_temperature_high[i];
		data_day_temperature_low[i] = dataFromIdentifier( 19 + i * 6 + 2 );
		qDebug() << "data_day_temperature_low[" << i << "]: " << data_day_temperature_low[i];
		qDebug() << "data_current_icon_code (orig): " << dataFromIdentifier( 19 + i * 6 + 3 );
		data_day_icon_code[i] = dataFromIdentifier( 19 + i * 6 + 3 ).toInt();
		for ( j=0; j<WEATHER_CODE_TRANSFORM_SIZE; j++ )
		{
			if ( QString::compare( dataFromIdentifier( 19 + i * 6 + 3 ), data_icon_transform_in_xml[j], Qt::CaseInsensitive ) == 0 )
			{
				data_day_icon_code[i] = data_icon_transform_out_xml[j].toInt();
			}
		}
		qDebug() << "data_day_icon_code[" << i << "]: " << data_day_icon_code[i];

		data_day_icon_text[i] = dataFromIdentifier( 19 + i * 6 + 5 );
		qDebug() << "data_day_icon_text[" << i << "]: " << data_day_icon_text[i];
	}

	// start download of icons
	icon_job_success_list[0] = false;
	KIO::Job *job_icon = KIO::get( KUrl( dataFromIdentifier( 11 ) ), KIO::Reload, KIO::HideProgressInfo );
	connect( job_icon, SIGNAL( data( KIO::Job *, const QByteArray & ) ), this, SLOT( iconDownloadedData( KIO::Job *, const QByteArray & ) ) );
	connect( job_icon, SIGNAL( result( KJob * ) ), this, SLOT( iconDownloadFinished( KJob * ) ) );
	icon_job_list[0] = job_icon;
	job_icon->start();
	for ( i=0; i<7; i++ )
	{
		icon_job_success_list[i+1] = false;
		job_icon = KIO::get( KUrl( dataFromIdentifier( 19 + i * 6 + 4 ) ), KIO::Reload, KIO::HideProgressInfo );
		connect( job_icon, SIGNAL( data( KIO::Job *, const QByteArray & ) ), this, SLOT( iconDownloadedData( KIO::Job *, const QByteArray & ) ) );
		connect( job_icon, SIGNAL( result( KJob * ) ), this, SLOT( iconDownloadFinished( KJob * ) ) );
		icon_job_list[i+1] = job_icon;
		job_icon->start();
	}

	data_update_time = KGlobal::locale()->formatTime( QTime::currentTime(), false, false );

	// shot, quick and dirty sanity check of downloaded value, if they're empty we better display nothing than that
	if ( !data_current_temperature.contains(QRegExp( "[0-9]" )) || data_current_temperature.length() > 6 /* too long temperature string? -21.5 has length=5 */ )
	{
		qDebug() << "Current temperature is invalid: Download didn't fail, but (all?) values are obviously invalid.";
//		data_update_time = "";
	}

	emit ( data_fetched() );
}

void Data_Provider::urlFollowCommandAppend( const QString command, const QString data, int identifier )
{
	if ( identifier < 0 || identifier > 7 ) return;

	url_follow_command_command[identifier] = command;
	url_follow_command_data_string[identifier] = data;
}

void Data_Provider::urlFollowCommandStartExecution()
{
	urlFollowCommandRunning = 0;

	urlFollowCommandStart( url_follow_command_command[urlFollowCommandRunning], urlFollowCommandRunning );
}


void Data_Provider::urlFollowCommandStart( const QString command, int identifier )
{
	if ( identifier < 0 || identifier > 7 ) return;

	delete url_follow_command_list[identifier];
	url_follow_command_list[identifier] = new KProcess;
	connect( url_follow_command_list[identifier], SIGNAL( started() ), this, SLOT( urlFollowCommandStarted() ) );
	connect( url_follow_command_list[identifier], SIGNAL( finished( int, QProcess::ExitStatus ) ), this, SLOT( urlFollowCommandFinished( int, QProcess::ExitStatus ) ) );

	if ( locale != "" ) url_follow_command_list[identifier]->setEnv( "LC_ALL", locale, true );

	url_follow_command_list[identifier]->setOutputChannelMode( KProcess::SeparateChannels );

	url_follow_command_list[identifier]->setShellCommand( QString( "sh -c " ) + KShell::quoteArg( command ) );
	url_follow_command_list[identifier]->start();
}

void Data_Provider::urlFollowCommandStarted()
{
	url_follow_command_list[urlFollowCommandRunning]->write( rawDataFromUrl( url_follow_command_data_string[urlFollowCommandRunning] ) );
	url_follow_command_list[urlFollowCommandRunning]->closeWriteChannel();
}

void Data_Provider::urlFollowCommandFinished( int exitCode, QProcess::ExitStatus exitStatus )
{
	Q_UNUSED( exitCode );
	Q_UNUSED( exitStatus );

	urlFollowCommandRunning++;

	if ( urlFollowCommandRunning < 0 || urlFollowCommandRunning >= 7 )
	{
		QString ret;
		QByteArray err;
		QByteArray arr;

		int i;
		for ( i=0; i<8; i++ )
		{
			if ( ( i == 0 && urlc_follow_xml != "" ) ||
				( i == 1 && url1_follow_xml != "" ) ||
				( i == 2 && url2_follow_xml != "" ) ||
				( i == 3 && url3_follow_xml != "" ) ||
				( i == 4 && url4_follow_xml != "" ) ||
				( i == 5 && url5_follow_xml != "" ) ||
				( i == 6 && url6_follow_xml != "" ) ||
				( i == 7 && url7_follow_xml != "" ) )
			{
				ret = "";

				err = url_follow_command_list[i]->readAllStandardError();
				if ( !err.isEmpty() ) qDebug() << "error running command on input data: " << err << endl;

				arr = url_follow_command_list[i]->readAllStandardOutput();

				if ( !arr.isEmpty() )
				{
					if ( encoding == "ascii" ) ret = QString::fromAscii( arr );
					else if ( encoding == "latin1" ) ret = QString::fromLatin1( arr );
					else if ( encoding == "local8bit" ) ret = QString::fromLocal8Bit( arr );
					else if ( encoding == "ucs4" ) ret = QString::fromUtf8( arr );
					else if ( encoding == "utf8" ) ret = QString::fromUtf8( arr );
					else if ( encoding == "utf16" ) ret = QString::fromUtf8( arr );
					else if ( encoding == "latin2" )
					{
						QTextCodec *inp = QTextCodec::codecForName( "ISO8859-2" );
						ret = inp->toUnicode(arr);
					}
					else ret = arr;
				}

				int pos;

				pos = ret.indexOf( "\r\n" );
				if ( pos != -1 ) ret = ret.left(pos);

				pos = ret.indexOf( "\n" );
				if ( pos != -1 ) ret = ret.left(pos);

				ret = ret.simplified();
				ret = ret.trimmed();

				if ( i == 0 ) urlc_new = ret;
				else if ( i == 1 ) url1_new = ret;
				else if ( i == 2 ) url2_new = ret;
				else if ( i == 3 ) url3_new = ret;
				else if ( i == 4 ) url4_new = ret;
				else if ( i == 5 ) url5_new = ret;
				else if ( i == 6 ) url6_new = ret;
				else if ( i == 7 ) url7_new = ret;
			}
		}

		url_follow_command_run = true;
		parseData();
	}
	else
	{
		urlFollowCommandStart( url_follow_command_command[urlFollowCommandRunning], urlFollowCommandRunning );
	}
}

void Data_Provider::dataCommandAppend( const QString command, const QString data, int identifier )
{
	if ( identifier < 0 || identifier > DATA_COMMAND_NUMBER ) return;

	data_command_command[identifier] = command;
	data_command_data_string[identifier] = data;
}

void Data_Provider::dataCommandStartExecution()
{
	dataCommandRunning = 0;
	dataCommandStart( data_command_command[dataCommandRunning], dataCommandRunning );
}

void Data_Provider::dataCommandStart( const QString command, int identifier )
{
	if ( identifier < 0 || identifier > DATA_COMMAND_NUMBER ) return;

	delete data_command_list[identifier];
	data_command_list[identifier] = new KProcess;
	connect( data_command_list[identifier], SIGNAL( started() ), this, SLOT( dataCommandStarted() ) );
	connect( data_command_list[identifier], SIGNAL( finished( int, QProcess::ExitStatus ) ), this, SLOT( dataCommandFinished( int, QProcess::ExitStatus ) ) );

	if ( locale != "" ) data_command_list[identifier]->setEnv( "LC_ALL", locale, true );

	data_command_list[identifier]->setOutputChannelMode( KProcess::SeparateChannels );

	data_command_list[identifier]->setShellCommand( QString( "sh -c " ) + KShell::quoteArg( command ) );
	data_command_list[identifier]->start();
}

void Data_Provider::dataCommandStarted()
{
	data_command_list[dataCommandRunning]->write( rawDataFromUrl( data_command_data_string[dataCommandRunning] ) );
	data_command_list[dataCommandRunning]->closeWriteChannel();
}

void Data_Provider::dataCommandFinished( int exitCode, QProcess::ExitStatus exitStatus )
{
	Q_UNUSED( exitCode );
	Q_UNUSED( exitStatus );

	dataCommandRunning++;

	if ( dataCommandRunning < 0 || dataCommandRunning >= DATA_COMMAND_NUMBER )
	{
		QString ret;
		QByteArray err;
		QByteArray arr;

		int i;
		for ( i=0; i<DATA_COMMAND_NUMBER; i++ )
		{
			ret = "";

			err = data_command_list[i]->readAllStandardError();
			if ( !err.isEmpty() ) qDebug() << "error running command on input data: " << err << endl;

			arr = data_command_list[i]->readAllStandardOutput();

			if ( !arr.isEmpty() )
			{
				if ( encoding == "ascii" ) ret = QString::fromAscii( arr );
				else if ( encoding == "latin1" ) ret = QString::fromLatin1( arr );
				else if ( encoding == "local8bit" ) ret = QString::fromLocal8Bit( arr );
				else if ( encoding == "ucs4" ) ret = QString::fromUtf8( arr );
				else if ( encoding == "utf8" ) ret = QString::fromUtf8( arr );
				else if ( encoding == "utf16" ) ret = QString::fromUtf8( arr );
				else if ( encoding == "latin2" )
				{
					QTextCodec *inp = QTextCodec::codecForName( "ISO8859-2" );
					ret = inp->toUnicode( arr );
				}
				else ret = arr;
			}

			int pos;

			pos = ret.indexOf( "\r\n" );
			if ( pos != -1 ) ret = ret.left(pos);

			pos = ret.indexOf( "\n" );
			if ( pos != -1 ) ret = ret.left(pos);

			ret = ret.simplified();
			ret = ret.trimmed();

			data_command_data_result[i] = ret;
		}

		data_command_run = true;
		parseData();
	}
	else
	{
		dataCommandStart( data_command_command[dataCommandRunning], dataCommandRunning );
	}
}

QString Data_Provider::dataFromIdentifier( int identifier )
{
	if ( identifier < 0 || identifier > DATA_COMMAND_NUMBER ) return "";

	return data_command_data_result[identifier];
}

void Data_Provider::dataDownloadedData( KIO::Job *job, const QByteArray &arr )
{
	int i;
	if ( data_job_list.size() != data_download_data_list.size() ) return;
	for ( i=0; i<data_job_list.size(); i++ )
	{
		if ( job == data_job_list[i] )
		{
			data_download_data_list[i] += arr;
		}
	}
}

void Data_Provider::dataDownloadFinished( KJob *job )
{
	int i;
	for ( i=0; i<data_job_list.size(); i++ )
	{
		if ( job == (KJob *)data_job_list[i] )
		{
			data_job_list[i] = NULL;
			if ( job->error() == 0 ) data_job_success_list[i] = true;
		}
	}
	bool all_null = true;
	for ( i=0; i<data_job_list.size(); i++ )
	{
		if ( data_job_list[i] )
		{
			all_null = false;
			break;
		}
	}
	if ( all_null )
	{
		parseData();
	}
}

void Data_Provider::urlFollowDownloadedData( KIO::Job *job, const QByteArray &arr )
{
	int i;
	if ( url_follow_job_list.size() != data_download_data_list.size() ) return;
	for ( i=0; i<data_download_data_list.size(); i++ )
	{
		if ( job == url_follow_job_list[i] )
		{
			data_download_data_list[i] += arr;
		}
	}
}

void Data_Provider::urlFollowDownloadFinished( KJob *job )
{
	int i;
	for ( i=0; i<url_follow_job_list.size(); i++ )
	{
		if ( job == (KJob *)url_follow_job_list[i] )
		{
			url_follow_job_list[i] = NULL;
			if ( job->error() == 0 ) url_follow_job_success_list[i] = true;
		}
	}
	bool all_null = true;
	for ( i=0; i<url_follow_job_list.size(); i++ )
	{
		if ( url_follow_job_list[i] )
		{
			all_null = false;
			break;
		}
	}
	if ( all_null )
	{
		url_followed = true;
		parseData();
	}
}

void Data_Provider::imageDownloadedData( KIO::Job *job, const QByteArray &arr )
{
	int i;
	for ( i=0; i<image_job_list.size(); i++ )
	{
		if ( job == image_job_list[i] )
		{
			data_image_list[i] += arr;
		}
	}
}

void Data_Provider::imageDownloadFinished( KJob *job )
{
	int i;
	for ( i=0; i<image_job_list.size(); i++ )
	{
		if ( job == (KJob *)image_job_list[i] )
		{
			if ( job->error() == 0 ) image_job_success_list[i] = true;
			image_job_list[i] = NULL;
		}
	}
	bool all_null = true;
	for ( i=0; i<image_job_list.size(); i++ )
	{
		if ( image_job_list[i] )
		{
			all_null = false;
			break;
		}
	}
	if ( all_null )
	{
		data_custom_image_list.clear();
		for ( i=0; i<image_job_success_list.size(); i++ )
		{
			if ( image_job_success_list[i] )
			{
				data_custom_image_list.push_back( data_image_list[i] );
			}
		}
		data_image_list.clear();

		emit( data_fetched() );
	}
}

void Data_Provider::iconDownloadedData( KIO::Job *job, const QByteArray &arr )
{
	int i;
	for ( i=0; i<icon_job_list.size(); i++ )
	{
		if ( job == icon_job_list[i] )
		{
			data_icon_list[i] += arr;
		}
	}
}

void Data_Provider::iconDownloadFinished( KJob *job )
{
	int i;
	for ( i=0; i<icon_job_list.size(); i++ )
	{
		if ( job == (KJob *)icon_job_list[i] )
		{
			icon_job_list[i] = NULL;
			if ( job->error() == 0 ) icon_job_success_list[i] = true;
		}
	}
	bool all_null = true;
	for ( i=0; i<icon_job_list.size(); i++ )
	{
		if ( icon_job_list[i] )
		{
			all_null = false;
			break;
		}
	}
	if ( all_null )
	{
		QImage icon_tmp = data_current_icon;
		if ( !data_current_icon.loadFromData( data_icon_list[0] ) || job->error() != 0 ) data_current_icon = icon_tmp;

		int j;
		for ( j=0; j<7; j++ )
		{
			icon_tmp = data_day_icon[j];
			if ( !data_day_icon[j].loadFromData( data_icon_list[j+1] ) || job->error() != 0 ) data_day_icon[j] = icon_tmp;
		}

		emit( data_fetched() );
	}
}

const QByteArray &Data_Provider::rawDataFromUrl( const QString &url )
{
	if ( url == "urlc" ) return data_download_data_list[0];
	else if ( url == "url1" ) return data_download_data_list[1];
	else if ( url == "url2" ) return data_download_data_list[2];
	else if ( url == "url3" ) return data_download_data_list[3];
	else if ( url == "url4" ) return data_download_data_list[4];
	else if ( url == "url5" ) return data_download_data_list[5];
	else if ( url == "url6" ) return data_download_data_list[6];
	else if ( url == "url7" ) return data_download_data_list[7];

	return data_download_data_list[0];
}

#include "plasma-cwp-data-provider.moc"
