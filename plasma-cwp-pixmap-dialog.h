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

#ifndef Plasma_CWP_Pixmap_Dialog_HEADER
#define Plasma_CWP_Pixmap_Dialog_HEADER

#include <QLabel>
#include <Plasma/Dialog>

class PixmapDialog : public Plasma::Dialog
{
	Q_OBJECT

	public:
		PixmapDialog( QWidget *parent = 0 );
		~PixmapDialog();

		void setResizable( bool resizable_new );

		void setPixmap( const QPixmap &pixmap );
		void setImage( const QImage &image );

	public slots:
		void resized();

	protected:
		void showEvent( QShowEvent *event );
		virtual void mousePressEvent( QMouseEvent *event );

	signals:
		void hidden();
		void showSignal();

	private:
		bool resizable;
		bool resized_on_show;
		QLabel *label;
		QPixmap contents_pixmap;

		void getLabelOffset( int &left, int &top, int &right, int &bottom );
};

#endif
