/*
 * https://github.com/peper0/qtasio

    The MIT License (MIT)

    Copyright (c) 2014 peper0

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

 * Adapted By liuzikai.
 */
#ifndef QASIOEVENTDISPATCHER_H
#define QASIOEVENTDISPATCHER_H

#include "QtCore/qabstracteventdispatcher.h"
#include "private/qabstracteventdispatcher_p.h"
#include <QAbstractEventDispatcher>

/*

#include "QtCore/qabstracteventdispatcher.h"
#include "QtCore/qlist.h"
#include "private/qabstracteventdispatcher_p.h"
#include "private/qcore_unix_p.h"
#include "private/qpodlist_p.h"
#include "QtCore/qvarlengtharray.h"

#if !defined(Q_OS_VXWORKS)
#  include <sys/time.h>
#  if (!defined(Q_OS_HPUX) || defined(__ia64)) && !defined(Q_OS_NACL)
#    include <sys/select.h>
#  endif
#endif

*/

class QAsioEventDispatcherPrivate;

namespace boost {
namespace asio {
class io_context;
typedef io_context io_service;
}
}

class Q_CORE_EXPORT QAsioEventDispatcher : public QAbstractEventDispatcher
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QAsioEventDispatcher)

public:
    explicit QAsioEventDispatcher(boost::asio::io_service &io_service, QObject *parent = 0);
    ~QAsioEventDispatcher();

    bool processEvents(QEventLoop::ProcessEventsFlags flags);
    bool hasPendingEvents();
    void registerSocketNotifier(QSocketNotifier *notifier) Q_DECL_FINAL;
    void unregisterSocketNotifier(QSocketNotifier *notifier) Q_DECL_FINAL;

    void registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object) Q_DECL_FINAL;
    bool unregisterTimer(int timerId) Q_DECL_FINAL;
    bool unregisterTimers(QObject *object) Q_DECL_FINAL;
    QList<TimerInfo> registeredTimers(QObject *object) const Q_DECL_FINAL;
    int remainingTime(int timerId) Q_DECL_FINAL;

    void wakeUp() Q_DECL_FINAL;
    void interrupt() Q_DECL_FINAL;
    //TODO:
    void flush();

protected:
    QAsioEventDispatcher(QAsioEventDispatcherPrivate &dd, QObject *parent = 0);

};

#endif // QASIOEVENTDISPATCHER_H
