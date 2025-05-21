#include "notification.h"
#include <QPainter>
#include <QVBoxLayout>
#include <QApplication>
#include <QScreen>

Notification::Notification(QWidget *parent)
    : QWidget(parent)
    , iconLabel(new QLabel(this))
    , messageLabel(new QLabel(this))
    , closeButton(new QPushButton(this))
    , timer(new QTimer(this))
    , animation(new QPropertyAnimation(this, "geometry"))
    , opacityEffect(new QGraphicsOpacityEffect(this))
{
    setupUI();

    setGraphicsEffect(opacityEffect);
    opacityEffect->setOpacity(1.0);

    connect(timer, &QTimer::timeout, this, &Notification::fadeOut);
    connect(closeButton, &QPushButton::clicked, this, &Notification::fadeOut);
}

void Notification::setupUI()
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::SubWindow);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(15, 10, 15, 10);
    mainLayout->setSpacing(10);

    // Настройка иконки
    iconLabel->setFixedSize(24, 24);
    iconLabel->setAttribute(Qt::WA_TranslucentBackground);
    mainLayout->addWidget(iconLabel);

    // Настройка сообщения
    messageLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    messageLabel->setWordWrap(true);
    messageLabel->setAttribute(Qt::WA_TranslucentBackground);
    messageLabel->setStyleSheet("QLabel { color: white; background: transparent; }");
    mainLayout->addWidget(messageLabel, 1);

    // Настройка кнопки закрытия
    closeButton->setFixedSize(16, 16);
    closeButton->setCursor(Qt::PointingHandCursor);
    closeButton->setStyleSheet(R"(
        QPushButton {
            border: none;
            background-color: transparent;
            color: white;
            font-family: Arial;
            font-weight: bold;
        }
        QPushButton:hover {
            color: rgba(255, 255, 255, 0.8);
        }
    )");
    closeButton->setText("✕");
    mainLayout->addWidget(closeButton);

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

    // Расчет позиции относительно родительского окна
    QWidget *parentWidget = qobject_cast<QWidget*>(parent());
    QRect parentGeometry = parentWidget ? parentWidget->rect() : QRect(0, 0, 800, 600);

    int x = parentGeometry.width() - width() - 20;
    int y = 20;

    // Анимация появления
    setGeometry(parentGeometry.width(), y, width(), height());
    show();

    animation->setDuration(300);
    animation->setStartValue(geometry());
    animation->setEndValue(QRect(x, y, width(), height()));
    animation->setEasingCurve(QEasingCurve::OutCubic);

    emit startedMoving();
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

    // Рисуем основное уведомление
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(QColor(backgroundRole())));
    painter.drawRoundedRect(rect(), 4, 4);

    // Рисуем цветную полоску слева
    QColor stripColor;
    switch (currentType) {
    case Success:
        stripColor = QColor(76, 175, 80);
        break;
    case Warning:
        stripColor = QColor(255, 193, 7);
        break;
    case Error:
        stripColor = QColor(244, 67, 54);
        break;
    case Info:
        stripColor = QColor(33, 150, 243);
        break;
    }

    painter.setBrush(stripColor);
    painter.drawRoundedRect(QRect(0, 0, 4, height()), 2, 2);
}

void Notification::moveDown(int offset)
{
    QWidget *parentWidget = qobject_cast<QWidget*>(parent());
    QRect parentGeometry = parentWidget ? parentWidget->rect() : QRect(0, 0, 800, 600);

    int x = parentGeometry.width() - width() - 20;
    int y = 20 + offset;

    QPropertyAnimation *moveAnimation = new QPropertyAnimation(this, "geometry");
    moveAnimation->setDuration(300);
    moveAnimation->setStartValue(geometry());
    moveAnimation->setEndValue(QRect(x, y, width(), height()));
    moveAnimation->setEasingCurve(QEasingCurve::OutCubic);

    connect(moveAnimation, &QPropertyAnimation::finished, moveAnimation, &QObject::deleteLater);

    emit startedMoving();
    moveAnimation->start();
}

void Notification::updatePosition(int offset)
{
    QWidget *parentWidget = qobject_cast<QWidget*>(parent());
    QRect parentGeometry = parentWidget ? parentWidget->rect() : QRect(0, 0, 800, 600);

    int x = parentGeometry.width() - width() - 20;
    int y = 20 + offset;

    move(x, y);
}
