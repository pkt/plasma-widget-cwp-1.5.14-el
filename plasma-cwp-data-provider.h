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

#ifndef Plasma_CWP_Data_Provider_HEADER
#define Plasma_CWP_Data_Provider_HEADER

#include <KIO/Job>
#include <KProcess>

#include <solid/networking.h>

#include "config.h"

const int WIND_TRANSFORM_SIZE = 500;
const int WEATHER_CODE_TRANSFORM_SIZE = 110;

const int DATA_COMMAND_NUMBER = 61;

class Data_Provider : public QObject
{
	Q_OBJECT

	public:
		Data_Provider( QObject *parent, const QString &path, const uint identifier );
		virtual ~Data_Provider();

		void get_weather_values( QString &data_location_location_tmp, QString &data_location_country_tmp, QString &data_sun_sunrise_tmp,
			QString &data_sun_sunset_tmp, QString &data_current_temperature_tmp, QString &data_current_temperature_felt_tmp,
			QString &data_current_wind_code_tmp, QString &data_current_wind_speed_tmp, QString &data_current_wind_tmp,
			QString &data_current_humidity_tmp, QString &data_current_dew_point_tmp,
			QString &data_current_visibility_tmp, QString &data_current_pressure_tmp, QString &data_current_uv_index_tmp,
			int &data_current_icon_code_tmp, QImage &data_current_icon_tmp,
			QString &data_current_icon_text_tmp, QString &data_current_rain_tmp, QString *data_day_name_tmp,
			QString *data_day_temperature_high_tmp, QString *data_day_temperature_low_tmp,
			int *data_day_icon_code_tmp, QImage *data_day_icon_tmp, QString *data_day_icon_text_tmp, QString &data_update_time_tmp,
			QString &data_provider_update_time_tmp, QString &tempType_tmp, QList<QByteArray> &data_custom_image_list_tmp,
			bool &last_download_succeeded_tmp, QString &provider_url_tmp );

		void set_config_values( const QString &updateFrequency_tmp, const QString &xmlFile_tmp, const QString &zip_tmp, const QString &feelsLike_tmp,
			const QString &humidity_tmp, const QString &wind_tmp, const QString &tempType_tmp, const QList<KUrl> &customImageList_tmp );

	signals:
		void data_fetched();
		void signalLoadLocalData();

	public slots:
		void reloadData();
		void solidNetworkingStatusChanged( Solid::Networking::Status status );

	private slots:
		void loadLocalData();
		void saveLocalData();

		void urlFollowCommandStart( const QString command, int identifier );
		void urlFollowCommandStarted();
		void urlFollowCommandFinished( int exitCode, QProcess::ExitStatus exitStatus );
		void dataCommandStart( const QString command, int identifier );
		void dataCommandStarted();
		void dataCommandFinished( int exitCode, QProcess::ExitStatus exitStatus );
		QString dataFromIdentifier( int identifier );

		void dataDownloadedData( KIO::Job *job, const QByteArray &arr );
		void dataDownloadFinished( KJob *job );
		void urlFollowDownloadedData( KIO::Job *job, const QByteArray &arr );
		void urlFollowDownloadFinished( KJob *job );
		void imageDownloadedData( KIO::Job *job, const QByteArray &arr );
		void imageDownloadFinished( KJob *job );
		void iconDownloadedData( KIO::Job *job, const QByteArray &arr );
		void iconDownloadFinished( KJob *job );

	private:
		void startDownloadData();

		int urlFollowCommandRunning;
		void urlFollowCommandAppend( const QString command, const QString data, int identifier );
		void urlFollowCommandStartExecution();

		int dataCommandRunning;
		void dataCommandAppend( const QString command, const QString data, int identifier );
		void dataCommandStartExecution();

		QTime collecting_data_start_time;
		QString local_data_path;
		uint local_data_identifier;

		bool url_followed;
		void parseData();

		const QByteArray &rawDataFromUrl( const QString &url );

		bool url_follow_command_run;
		bool data_command_run;
		QList<KProcess*> url_follow_command_list;
		QList<QString> url_follow_command_data_string;
		QList<QString> url_follow_command_command;
		QList<KProcess*> data_command_list;
		QList<QString> data_command_data_string;
		QList<QString> data_command_command;
		QList<QString> data_command_data_result;

		QString urlc_new;
		QString url1_new;
		QString url2_new;
		QString url3_new;
		QString url4_new;
		QString url5_new;
		QString url6_new;
		QString url7_new;

		bool last_download_succeeded;
		QList<KIO::Job*> data_job_list;
		QList<KIO::Job*> url_follow_job_list;
		QList<KIO::Job*> image_job_list;
		QList<KIO::Job*> icon_job_list;

		QList<bool> data_job_success_list;
		QList<bool> url_follow_job_success_list;
		QList<bool> image_job_success_list;
		QList<bool> icon_job_success_list;

		QList<QByteArray> data_download_data_list;
		QList<QByteArray> data_image_list;
		QList<QByteArray> data_icon_list;

		QList<QByteArray> data_custom_image_list;

		QString updateFrequency;
		QString xmlFile;
		QString zip;
		QString feelsLike;
		QString humidity;
		QString wind;
		QString tempType;

		QList<KUrl> customImageList;

		QString locale;
		QString encoding;

		QString data_update_time;

		QString data_provider_update_time;

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

		// xml file stuff
		QString urlc_prefix_xml;
		QString urlc_suffix_xml;

		QString url1_prefix_xml;
		QString url1_suffix_xml;

		QString url2_prefix_xml;
		QString url2_suffix_xml;

		QString url3_prefix_xml;
		QString url3_suffix_xml;

		QString url4_prefix_xml;
		QString url4_suffix_xml;

		QString url5_prefix_xml;
		QString url5_suffix_xml;

		QString url6_prefix_xml;
		QString url6_suffix_xml;

		QString url7_prefix_xml;
		QString url7_suffix_xml;

		QString urlc_follow_xml;
		QString url1_follow_xml;
		QString url2_follow_xml;
		QString url3_follow_xml;
		QString url4_follow_xml;
		QString url5_follow_xml;
		QString url6_follow_xml;
		QString url7_follow_xml;

		QString locale_xml;
		QString encoding_xml;

		QString data_location_url_xml;
		QString data_location_location_xml;
		QString data_location_country_xml;

		QString data_sun_url_xml;
		QString data_sun_sunrise_xml;
		QString data_sun_sunset_xml;

		QString data_current_temperature_url_xml;
		QString data_provider_update_time_xml;
		QString data_current_temperature_xml;
		QString data_current_temperature_felt_xml;
		QString data_current_wind_url_xml;
		QString data_current_wind_code_xml;
		QString data_current_wind_speed_xml;
		QString data_current_wind_xml;
		QString data_current_icon_url_xml;
		QString data_current_icon_code_xml;
		QString data_current_icon_xml;
		QString data_current_icon_text_xml;
		QString data_current_additional_url_xml;
		QString data_current_humidity_xml;
		QString data_current_rain_xml;
		QString data_current_dew_point_xml;
		QString data_current_visibility_xml;
		QString data_current_pressure_xml;
		QString data_current_uv_index_xml;

		QString data_day_url_xml[7];
		QString data_day_name_xml[7];
		QString data_day_temperature_high_xml[7];
		QString data_day_temperature_low_xml[7];
		QString data_day_icon_code_xml[7];
		QString data_day_icon_xml[7];
		QString data_day_icon_text_xml[7];

		QString data_icon_transform_in_xml[WEATHER_CODE_TRANSFORM_SIZE];
		QString data_icon_transform_out_xml[WEATHER_CODE_TRANSFORM_SIZE];

		QString data_wind_transform_in_xml[WIND_TRANSFORM_SIZE];
		QString data_wind_transform_out_xml[WIND_TRANSFORM_SIZE];
};

#endif
