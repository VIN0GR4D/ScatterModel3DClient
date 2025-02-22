#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QPushButton>

class Notification : public QWidget
{
    Q_OBJECT

public:
    enum Type {
        Info,
        Success,
        Warning,
        Error
    };

    explicit Notification(QWidget *parent = nullptr);
    void showMessage(const QString &message, Type type = Info, int duration = 3000);

public slots:
    void moveDown(int offset);
    void updatePosition(int offset);

protected:
    void paintEvent(QPaintEvent *event) override;

signals:
    void startedMoving();

private:
    QLabel *iconLabel;
    QLabel *messageLabel;
    QPushButton *closeButton;
    QTimer *timer;
    QPropertyAnimation *animation;
    QGraphicsOpacityEffect *opacityEffect;
    Type currentType;

    void setupUI();
    void setStyleForType(Type type);
    void startTimer(int duration);
    void fadeOut();
};

#endif // NOTIFICATION_H
