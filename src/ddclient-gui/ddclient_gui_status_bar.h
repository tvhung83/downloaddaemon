/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef DDCLIENT_GUI_STATUS_BAR_H
#define DDCLIENT_GUI_STATUS_BAR_H

#include <QStyledItemDelegate>


class status_bar : public QStyledItemDelegate{
    public:
        /** Defaultconstructor */
        status_bar(QObject *parent);
        /** Destructor */
        ~status_bar();

        /** standardmethod sizeHint with default parameters */
        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;

        /** standardmethod paint with default parameters */
        void paint(QPainter* painter, const QStyleOptionViewItem &option, const QModelIndex& index) const;
};

#endif // DDCLIENT_GUI_STATUS_BAR_H
