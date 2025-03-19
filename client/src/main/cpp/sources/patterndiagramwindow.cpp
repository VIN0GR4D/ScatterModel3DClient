#include "PatternDiagramWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QtMath>
#include <QDebug>
#include <algorithm>

// Вспомогательный класс для области рисования
class DiagramCanvas : public QWidget {
public:
    DiagramCanvas(PatternDiagramWindow* owner, QWidget* parent = nullptr)
        : QWidget(parent), m_owner(owner) {}

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Рисуем фон
        QRect rect = this->rect();
        painter.fillRect(rect, Qt::white);
        painter.setPen(Qt::black);
        painter.drawRect(rect.adjusted(0, 0, -1, -1));

        // Рисуем диаграмму направленности
        if (m_owner->getProjectionType() == 0) {
            m_owner->drawPolarPattern(painter, rect);
        } else {
            m_owner->draw3DPattern(painter, rect);
        }
    }

private:
    PatternDiagramWindow* m_owner;
};

PatternDiagramWindow::PatternDiagramWindow(QWidget *parent)
    : QDialog(parent),
    m_scale(1.0),
    m_azimuthAngle(0.0),
    m_elevationAngle(30.0),
    m_normalized(true),
    m_logarithmicScale(false),
    m_fillPattern(true),
    m_projectionType(0),
    m_sliceAngle(0),
    m_maxValue(1.0),
    m_minValue(0.0)
{
    setWindowTitle("Диаграмма направленности");
    resize(800, 600);

    setupUI();
}

void PatternDiagramWindow::setupUI()
{
    // Используем вертикальный компоновщик для основной структуры окна
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Создаем виджет для области рисования
    m_drawingArea = new DiagramCanvas(this);
    m_drawingArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_drawingArea->setMinimumHeight(400); // Минимальная высота для области рисования

    // Отдельный слой для элементов управления
    QWidget *controlPanel = new QWidget;
    m_controlsLayout = new QVBoxLayout(controlPanel);

    // Первый ряд элементов управления: проекция и нормализация
    QHBoxLayout *row1Layout = new QHBoxLayout();
    row1Layout->addWidget(new QLabel("Тип проекции:"));
    m_projectionCombo = new QComboBox(this);
    m_projectionCombo->addItem("Полярная проекция");
    m_projectionCombo->addItem("3D проекция");
    row1Layout->addWidget(m_projectionCombo);

    m_normalizeCheck = new QCheckBox("Нормализация", this);
    m_normalizeCheck->setChecked(m_normalized);
    row1Layout->addWidget(m_normalizeCheck);

    m_logScaleCheck = new QCheckBox("Логарифмический масштаб", this);
    m_logScaleCheck->setChecked(m_logarithmicScale);
    row1Layout->addWidget(m_logScaleCheck);

    m_fillCheck = new QCheckBox("Заполнение", this);
    m_fillCheck->setChecked(m_fillPattern);
    row1Layout->addWidget(m_fillCheck);

    // Второй ряд: угол среза
    QHBoxLayout *row2Layout = new QHBoxLayout();
    m_sliceLabel = new QLabel("Угол среза: 0°", this);
    row2Layout->addWidget(m_sliceLabel);

    m_sliceSlider = new QSlider(Qt::Horizontal, this);
    m_sliceSlider->setRange(0, 360);
    m_sliceSlider->setValue(0);
    m_sliceSlider->setTickInterval(45);
    m_sliceSlider->setTickPosition(QSlider::TicksBelow);
    row2Layout->addWidget(m_sliceSlider);

    // Третий ряд: кнопки
    QHBoxLayout *row3Layout = new QHBoxLayout();
    m_resetButton = new QPushButton("Сбросить вид", this);
    row3Layout->addWidget(m_resetButton);

    m_saveButton = new QPushButton("Сохранить изображение", this);
    row3Layout->addWidget(m_saveButton);

    // Добавляем все ряды в панель управления
    m_controlsLayout->addLayout(row1Layout);
    m_controlsLayout->addLayout(row2Layout);
    m_controlsLayout->addLayout(row3Layout);

    // Добавляем область рисования и панель управления в основной компоновщик
    mainLayout->addWidget(m_drawingArea, 1);
    mainLayout->addWidget(controlPanel, 0);

    // Соединяем сигналы с слотами
    connect(m_saveButton, &QPushButton::clicked, this, &PatternDiagramWindow::saveImage);
    connect(m_resetButton, &QPushButton::clicked, this, &PatternDiagramWindow::resetView);
    connect(m_projectionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PatternDiagramWindow::updateProjectionType);
    connect(m_normalizeCheck, &QCheckBox::toggled,
            this, &PatternDiagramWindow::toggleNormalization);
    connect(m_logScaleCheck, &QCheckBox::toggled,
            this, &PatternDiagramWindow::toggleLogScale);
    connect(m_fillCheck, &QCheckBox::toggled,
            this, &PatternDiagramWindow::toggleFillMode);
    connect(m_sliceSlider, &QSlider::valueChanged,
            this, &PatternDiagramWindow::updateSliceAngle);
}

void PatternDiagramWindow::setData(const QVector<QVector<double>>& data)
{
    m_data = data;
    calculateMinMax();
    // Обновляем область рисования вместо всего окна
    if (m_drawingArea) {
        m_drawingArea->update();
    }
}

void PatternDiagramWindow::calculateMinMax()
{
    if (m_data.isEmpty()) return;

    m_maxValue = std::numeric_limits<double>::lowest();
    m_minValue = std::numeric_limits<double>::max();

    for (const auto& row : m_data) {
        for (double val : row) {
            if (val > m_maxValue) m_maxValue = val;
            if (val < m_minValue) m_minValue = val;
        }
    }

    // Избегаем деления на ноль
    if (qFuzzyCompare(m_maxValue, m_minValue)) {
        m_maxValue += 1.0;
    }
}

void PatternDiagramWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    // В обновленной реализации метода paintEvent(),
    // отрисовка должна происходить только в области drawingArea,
    // но не в основном окне.

    // Вместо этого, мы оставляем эту функцию пустой,
    // так как основная отрисовка будет происходить в paintEvent
    // объекта drawingArea, переопределенном в setupUI()
}

void PatternDiagramWindow::drawPolarPattern(QPainter &painter, const QRect &rect)
{
    if (m_data.isEmpty()) return;

    int centerX = rect.center().x();
    int centerY = rect.center().y();
    int radius = qMin(rect.width(), rect.height()) / 2 - 10;

    // Рисуем окружности сетки
    painter.setPen(QPen(Qt::lightGray, 1, Qt::DashLine));
    for (int r = radius / 4; r <= radius; r += radius / 4) {
        painter.drawEllipse(QPoint(centerX, centerY), r, r);
    }

    // Рисуем радиальные линии
    for (int angle = 0; angle < 360; angle += 30) {
        double radians = qDegreesToRadians(static_cast<double>(angle));
        int x = centerX + radius * cos(radians);
        int y = centerY + radius * sin(radians);
        painter.drawLine(centerX, centerY, x, y);

        // Добавляем метки углов
        int textX = centerX + (radius + 15) * cos(radians);
        int textY = centerY + (radius + 15) * sin(radians);
        painter.drawText(textX - 10, textY + 5, QString::number(angle) + "°");
    }

    // Рисуем диаграмму направленности
    int rowCount = m_data.size();
    int colCount = m_data[0].size();

    QVector<QPointF> points;

    // Собираем точки для указанного среза
    for (int i = 0; i < colCount; ++i) {
        double angle = 360.0 * i / colCount;
        double radians = qDegreesToRadians(angle);

        // Берем данные из ряда, соответствующего выбранному срезу
        int rowIndex = qMin(m_sliceAngle * rowCount / 360, rowCount - 1);
        double value = normalizeValue(m_data[rowIndex][i]);

        double r = value * radius;
        double x = centerX + r * cos(radians);
        double y = centerY + r * sin(radians);

        points.append(QPointF(x, y));
    }

    // Замыкаем контур, добавляя первую точку в конец
    if (!points.isEmpty()) {
        points.append(points.first());
    }

    // Рисуем контур диаграммы
    painter.setPen(QPen(Qt::blue, 2));

    if (m_fillPattern) {
        // Заполняем диаграмму с полупрозрачным цветом
        QColor fillColor = Qt::blue;
        fillColor.setAlphaF(0.3);
        painter.setBrush(fillColor);
    } else {
        painter.setBrush(Qt::NoBrush);
    }

    if (points.size() > 1) {
        painter.drawPolygon(points.data(), points.size());
    }

    // Рисуем оси
    painter.setPen(QPen(Qt::black, 2));
    painter.drawLine(centerX - radius, centerY, centerX + radius, centerY);
    painter.drawLine(centerX, centerY - radius, centerX, centerY + radius);

    // Добавляем легенду
    painter.drawText(rect.right() - 150, rect.top() + 20, "Нормализованное значение");
    for (int i = 0; i <= 10; ++i) {
        double val = i / 10.0;
        int y = rect.top() + 30 + i * 20;
        painter.fillRect(rect.right() - 150, y, 20, 15, getColorForValue(val));
        painter.drawRect(rect.right() - 150, y, 20, 15);
        painter.drawText(rect.right() - 120, y + 12, QString::number(val, 'f', 1));
    }
}

void PatternDiagramWindow::draw3DPattern(QPainter &painter, const QRect &rect)
{
    if (m_data.isEmpty()) return;

    int centerX = rect.center().x();
    int centerY = rect.center().y();
    int size = qMin(rect.width(), rect.height()) / 2 - 20;

    // Применяем трансформацию для 3D вращения
    painter.save();
    painter.translate(centerX, centerY);

    // Рисуем оси координат
    painter.setPen(QPen(Qt::red, 2));
    painter.drawLine(0, 0, size, 0);
    painter.drawText(size + 5, 0, "X");

    painter.setPen(QPen(Qt::green, 2));
    painter.drawLine(0, 0, 0, -size);
    painter.drawText(0, -size - 5, "Y");

    painter.setPen(QPen(Qt::blue, 2));
    QPointF zEnd = sphericalToCartesian(size, m_elevationAngle, m_azimuthAngle);
    painter.drawLine(0, 0, zEnd.x(), zEnd.y());
    painter.drawText(zEnd.x() + 5, zEnd.y() + 5, "Z");

    // Рисуем сетку сферы
    painter.setPen(QPen(Qt::lightGray, 1, Qt::DashLine));

    // Горизонтальные круги (параллели)
    for (int elevation = 0; elevation <= 90; elevation += 15) {
        QVector<QPointF> points;
        for (int azimuth = 0; azimuth < 360; azimuth += 5) {
            points.append(sphericalToCartesian(size, elevation, azimuth));
        }
        points.append(points.first()); // Замыкаем контур

        if (points.size() > 1) {
            painter.drawPolyline(points.data(), points.size());
        }
    }

    // Вертикальные круги (меридианы)
    for (int azimuth = 0; azimuth < 360; azimuth += 30) {
        QVector<QPointF> points;
        for (int elevation = 0; elevation <= 90; elevation += 5) {
            points.append(sphericalToCartesian(size, elevation, azimuth));
        }

        if (points.size() > 1) {
            painter.drawPolyline(points.data(), points.size());
        }
    }

    // Рисуем диаграмму направленности в 3D
    int rows = m_data.size();
    int cols = m_data[0].size();

    painter.setPen(QPen(Qt::blue, 1));

    for (int i = 0; i < rows; ++i) {
        double elevation = 90.0 * i / (rows - 1);
        QVector<QPointF> points;

        for (int j = 0; j < cols; ++j) {
            double azimuth = 360.0 * j / cols;
            double value = normalizeValue(m_data[i][j]);
            points.append(sphericalToCartesian(value * size, elevation, azimuth));
        }

        // Замыкаем контур
        if (!points.isEmpty()) {
            points.append(points.first());
        }

        if (points.size() > 1) {
            if (m_fillPattern) {
                QColor fillColor = getColorForValue(0.5);
                fillColor.setAlphaF(0.3);
                painter.setBrush(fillColor);
            } else {
                painter.setBrush(Qt::NoBrush);
            }
            painter.drawPolyline(points.data(), points.size());
        }
    }

    painter.restore();

    // Добавляем легенду
    painter.drawText(rect.right() - 150, rect.top() + 20, "Нормализованное значение");
    for (int i = 0; i <= 10; ++i) {
        double val = i / 10.0;
        int y = rect.top() + 30 + i * 20;
        painter.fillRect(rect.right() - 150, y, 20, 15, getColorForValue(val));
        painter.drawRect(rect.right() - 150, y, 20, 15);
        painter.drawText(rect.right() - 120, y + 12, QString::number(val, 'f', 1));
    }
}

QColor PatternDiagramWindow::getColorForValue(double value) const
{
    // Преобразуем значение в цвет от синего (0) до красного (1)
    int hue = static_cast<int>(240 - value * 240);
    return QColor::fromHsv(hue, 255, 255);
}

double PatternDiagramWindow::normalizeValue(double value) const
{
    if (m_logarithmicScale) {
        // Для логарифмического масштаба, защищаем от нулевых и отрицательных значений
        double minPositive = qMax(m_minValue, 1e-10);
        double logMin = qLn(minPositive);
        double logMax = qLn(qMax(m_maxValue, minPositive * 1.1));
        double logVal = qLn(qMax(value, minPositive));

        return (logVal - logMin) / (logMax - logMin);
    } else {
        // Для линейного масштаба
        return (value - m_minValue) / (m_maxValue - m_minValue);
    }
}

QPointF PatternDiagramWindow::polarToCartesian(double radius, double angle) const
{
    double radians = qDegreesToRadians(angle);
    return QPointF(radius * qCos(radians), radius * qSin(radians));
}

QPointF PatternDiagramWindow::sphericalToCartesian(double radius, double theta, double phi) const
{
    // theta - угол отклонения от оси Z (0-90 градусов)
    // phi - азимутальный угол в плоскости XY (0-360 градусов)
    double thetaRad = qDegreesToRadians(theta);
    double phiRad = qDegreesToRadians(phi);

    // Применяем поворот для угла обзора
    double azimuthRad = qDegreesToRadians(m_azimuthAngle);
    double elevationRad = qDegreesToRadians(m_elevationAngle);

    double x = radius * qSin(thetaRad) * qCos(phiRad);
    double y = radius * qSin(thetaRad) * qSin(phiRad);
    double z = radius * qCos(thetaRad);

    // Применяем поворот вокруг оси Y (для угла места)
    double x2 = x * qCos(elevationRad) + z * qSin(elevationRad);
    double y2 = y;
    double z2 = -x * qSin(elevationRad) + z * qCos(elevationRad);

    // Применяем поворот вокруг оси Z (для азимута)
    double x3 = x2 * qCos(azimuthRad) - y2 * qSin(azimuthRad);
    double y3 = x2 * qSin(azimuthRad) + y2 * qCos(azimuthRad);

    // Для 2D отображения используем только X и Y координаты, Z используется для определения видимости
    return QPointF(x3, -y3); // Инвертируем Y для соответствия экранным координатам
}

void PatternDiagramWindow::mousePressEvent(QMouseEvent *event)
{
    // Проверяем, находится ли курсор над областью рисования
    if (m_drawingArea && m_drawingArea->rect().contains(event->pos() - m_drawingArea->pos())) {
        m_lastMousePos = event->pos();
        event->accept();
    } else {
        QDialog::mousePressEvent(event);
    }
}

void PatternDiagramWindow::mouseMoveEvent(QMouseEvent *event)
{
    // Обрабатываем только если исходное нажатие было в области рисования
    if (event->buttons() & Qt::LeftButton && !m_lastMousePos.isNull()) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_azimuthAngle += delta.x() * 0.5;
        m_elevationAngle = qBound(0.0, m_elevationAngle - delta.y() * 0.5, 90.0);

        m_lastMousePos = event->pos();

        if (m_drawingArea) {
            m_drawingArea->update(); // Обновить только область рисования
        }

        event->accept();
    } else {
        QDialog::mouseMoveEvent(event);
    }
}

void PatternDiagramWindow::wheelEvent(QWheelEvent *event)
{
    // Проверяем, находится ли курсор над областью рисования
    if (m_drawingArea && m_drawingArea->rect().contains(event->position().toPoint() - m_drawingArea->pos())) {
        QPoint numDegrees = event->angleDelta() / 8;
        if (!numDegrees.isNull()) {
            double factor = 1.0 + numDegrees.y() / 360.0;
            m_scale = qBound(0.1, m_scale * factor, 10.0);

            if (m_drawingArea) {
                m_drawingArea->update(); // Обновить только область рисования
            }

            event->accept();
        }
    } else {
        QDialog::wheelEvent(event);
    }
}

void PatternDiagramWindow::saveImage()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Сохранить изображение"), "", tr("PNG Image (*.png)"));
    if (fileName.isEmpty()) return;

    // Получаем область рисования (первый виджет в макете)
    QWidget* drawingArea = nullptr;
    if (layout()) {
        QLayoutItem* item = layout()->itemAt(0);
        if (item && item->widget()) {
            drawingArea = item->widget();
        }
    }

    if (!drawingArea) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось получить область рисования."));
        return;
    }

    // Создаем изображение размером с область рисования
    QPixmap pixmap(drawingArea->size());
    pixmap.fill(Qt::white);

    QPainter painter(&pixmap);

    // Рисуем диаграмму непосредственно на pixmap
    if (m_projectionType == 0) {
        drawPolarPattern(painter, drawingArea->rect());
    } else {
        draw3DPattern(painter, drawingArea->rect());
    }

    // Добавляем информацию о настройках
    QString settings;
    settings += m_normalized ? "Нормализовано " : "";
    settings += m_logarithmicScale ? "Логарифмическая шкала " : "Линейная шкала ";
    settings += QString("Угол среза: %1°").arg(m_sliceAngle);

    painter.setPen(Qt::black);
    QFont font = painter.font();
    font.setPointSize(10);
    painter.setFont(font);
    painter.drawText(10, 20, settings);

    if (pixmap.save(fileName)) {
        QMessageBox::information(this, tr("Успех"), tr("Изображение успешно сохранено."));
    } else {
        QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось сохранить изображение."));
    }
}

void PatternDiagramWindow::resetView()
{
    m_scale = 1.0;
    m_azimuthAngle = 0.0;
    m_elevationAngle = 30.0;
    m_sliceSlider->setValue(0);
    if (m_drawingArea) {
        m_drawingArea->update();
    }
}

void PatternDiagramWindow::toggleNormalization(bool normalized)
{
    m_normalized = normalized;
    if (m_drawingArea) {
        m_drawingArea->update();
    }
}

void PatternDiagramWindow::toggleLogScale(bool useLog)
{
    m_logarithmicScale = useLog;
    if (m_drawingArea) {
        m_drawingArea->update();
    }
}

void PatternDiagramWindow::toggleFillMode(bool fill)
{
    m_fillPattern = fill;
    if (m_drawingArea) {
        m_drawingArea->update();
    }
}

void PatternDiagramWindow::updateProjectionType(int index)
{
    m_projectionType = index;
    if (m_drawingArea) {
        m_drawingArea->update();
    }
}

void PatternDiagramWindow::updateSliceAngle(int angle)
{
    m_sliceAngle = angle;
    m_sliceLabel->setText(QString("Угол среза: %1°").arg(angle));
    if (m_drawingArea) {
        m_drawingArea->update();
    }
}

void PatternDiagramWindow::setAzimuthAngle(double angle)
{
    m_azimuthAngle = angle;
    if (m_drawingArea) {
        m_drawingArea->update();
    }
}

void PatternDiagramWindow::setElevationAngle(double angle)
{
    m_elevationAngle = qBound(0.0, angle, 90.0);
    if (m_drawingArea) {
        m_drawingArea->update();
    }
}
