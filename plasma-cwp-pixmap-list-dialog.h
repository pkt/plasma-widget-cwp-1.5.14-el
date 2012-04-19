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

#ifndef Plasma_CWP_Pixmap_List_Dialog_HEADER
#define Plasma_CWP_Pixmap_List_Dialog_HEADER

#include <Plasma/Dialog>

class QBuffer;
class QLabel;

class PixmapListDialog : public Plasma::Dialog
{
	Q_OBJECT

	public:
		PixmapListDialog( QWidget *parent = 0 );
		~PixmapListDialog();

		void setResizable( bool resizable_new );

		void setImageList( const QList<QByteArray> &list );
		void setNameList( const QStringList &list );
		void setCurrentImage( const int current );
		void setFont( const QFont &font );

	public slots:
		void resized();

	protected:
		void showEvent( QShowEvent *event );
		bool eventFilter(QObject *obj, QEvent *event);

	signals:
		void hidden();
		void currentImageChanged( int image );

	private:
		bool resizable;
		QPixmap left_icon_pixmap;
		QPixmap right_icon_pixmap;
		QPixmap left_icon_pixmap_lighter;
		QPixmap right_icon_pixmap_lighter;
		bool left_icon_hover;
		bool right_icon_hover;
		QLabel *label;
		QLabel *background_label;
		QLabel *left_icon;
		QLabel *title;
		QLabel *right_icon;
		QList<QByteArray> contents_image_list;
		QStringList contents_name_list;
		int current_image;
		QBuffer* movie_buffer;

		void getLabelOffset( int &left, int &top, int &right, int &bottom );
		void setMovie( const QByteArray& image );
};

#endif
