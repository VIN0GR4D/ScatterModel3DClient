#include "notification.h"
#include <QPainter>
#include <QVBoxLayout>
#include <QApplication>
#include <QScreen>

Notification::Notification(QWidget *parent)
    : QWidget(parent)
    , iconLabel(new QLabel(this))
    , messageLabel(new QLabel(this))
    , timer(new QTimer(this))
    , animation(new QPropertyAnimation(this, "geometry"))
    , opacityEffect(new QGraphicsOpacityEffect(this))
{
    setupUI();

    setGraphicsEffect(opacityEffect);
    opacityEffect->setOpacity(1.0);

    connect(timer, &QTimer::timeout, this, &Notification::fadeOut);
}

void Notification::setupUI()
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);
    mainLayout->setSpacing(10);

    // Настройка иконки
    iconLabel->setFixedSize(24, 24);
    mainLayout->addWidget(iconLabel);

    // Настройка сообщения
    messageLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    messageLabel->setWordWrap(true);
    messageLabel->setStyleSheet("QLabel { color: white; }");
    mainLayout->addWidget(messageLabel, 1);

    setFixedWidth(300);
    setMinimumHeight(50);
}

void Notification::setStyleForType(Type type)
{
    QString backgroundColor;
    QString iconPath;
    currentType = type;

    switch (type) {
    case Info:
        backgroundColor = "#2196F3";
        iconPath = ":/information.png";
        break;
    case Success:
        backgroundColor = "#4CAF50";
        iconPath = ":/success.png";
        break;
    case Warning:
        backgroundColor = "#FFC107";
        iconPath = ":/warning.png";
        break;
    case Error:
        backgroundColor = "#F44336";
        iconPath = ":/error.png";
        break;
    }

    // Загрузка и установка иконки
    QPixmap icon(iconPath);
    if (!icon.isNull()) {
        icon = icon.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        iconLabel->setPixmap(icon);
    }

    setStyleSheet(QString("Notification { background-color: %1; border-radius: 4px; }").arg(backgroundColor));
}

void Notification::showMessage(const QString &message, Type type, int duration)
{
    messageLabel->setText(message);
    setStyleForType(type);

    adjustSize();

    // Позиционирование уведомления в правом верхнем углу
    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();

    int x = screenGeometry.width() - width() - 20;
    int y = 20;

    // Анимация появления
    setGeometry(screenGeometry.width(), y, width(), height());
    show();

    animation->setDuration(300);
    animation->setStartValue(geometry());
    animation->setEndValue(QRect(x, y, width(), height()));
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->start();

    startTimer(duration);
}

void Notification::startTimer(int duration)
{
    timer->start(duration);
}

void Notification::fadeOut()
{
    timer->stop();

    QPropertyAnimation *fadeAnimation = new QPropertyAnimation(opacityEffect, "opacity");
    fadeAnimation->setDuration(500);
    fadeAnimation->setStartValue(1.0);
    fadeAnimation->setEndValue(0.0);
    fadeAnimation->setEasingCurve(QEasingCurve::OutCubic);

    connect(fadeAnimation, &QPropertyAnimation::finished, this, [this]() {
        hide();
        deleteLater();
    });

    fadeAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

void Notification::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Добавление тени
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 50));
    painter.drawRoundedRect(rect().adjusted(2, 2, 2, 2), 4, 4);

    // Основное уведомление
    painter.setBrush(QBrush(QColor(backgroundRole())));
    painter.drawRoundedRect(rect(), 4, 4);
}
