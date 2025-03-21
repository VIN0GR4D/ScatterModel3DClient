#include "PatternDiagramWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QtMath>
#include <QDebug>
#include <algorithm>

// Вспомогательный класс для области рисования диаграммы направленности
// Использует владельца (PatternDiagramWindow) для получения данных и отрисовки
class DiagramCanvas : public QWidget {
public:
    DiagramCanvas(PatternDiagramWindow* owner, QWidget* parent = nullptr)
        : QWidget(parent), m_owner(owner) {}

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Рисуем фон и рамку
        QRect rect = this->rect();
        painter.fillRect(rect, Qt::white);
        painter.setPen(Qt::black);
        painter.drawRect(rect.adjusted(0, 0, -1, -1));

        // Выбираем тип диаграммы в зависимости от настроек:
        // 0 - полярная (2D), 1 - сферическая (3D)
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
    m_scale(1.0),              // Начальный масштаб отображения
    m_azimuthAngle(0.0),       // Начальный азимутальный угол просмотра (в градусах)
    m_elevationAngle(30.0),    // Начальный угол места (в градусах)
    m_normalized(true),        // Нормализация данных включена по умолчанию
    m_logarithmicScale(false), // Логарифмический масштаб отключен по умолчанию
    m_fillPattern(true),       // Заливка контуров включена по умолчанию
    m_projectionType(0),       // Начальный тип проекции - полярная (2D)
    m_sliceAngle(0),           // Начальный угол среза для 2D-проекции
    m_maxValue(1.0),           // Инициализация значений для предотвращения деления на ноль
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

    // Создаем специализированный виджет для области рисования с передачей ссылки на текущий класс
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
    // stretch factor 1 для области рисования, чтобы она занимала всё доступное пространство
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

/**
 * Вычисляет минимальное и максимальное значения в данных для нормализации
 * Эти значения используются в normalizeValue() для приведения данных к диапазону [0, 1]
 */
void PatternDiagramWindow::calculateMinMax()
{
    if (m_data.isEmpty()) return;

    // Инициализируем максимум наименьшим возможным значением, а минимум - наибольшим
    m_maxValue = std::numeric_limits<double>::lowest();
    m_minValue = std::numeric_limits<double>::max();

    // Находим фактические минимум и максимум в массиве данных
    for (const auto& row : m_data) {
        for (double val : row) {
            if (val > m_maxValue) m_maxValue = val;
            if (val < m_minValue) m_minValue = val;
        }
    }

    // Защита от случая, когда все значения одинаковы (деление на ноль)
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

/**
 * Отрисовывает диаграмму направленности в полярной проекции (2D)
 *
 * Математическая основа:
 * 1. Расчет координат точек в полярной системе координат
 * 2. Преобразование полярных координат в декартовы для отображения:
 *    x = centerX + r * cos(θ)
 *    y = centerY + r * sin(θ)
 * Где:
 *   - r - нормализованное значение (интенсивность) * радиус области
 *   - θ - угол в радианах
 *   - centerX, centerY - центр области отображения
 */
void PatternDiagramWindow::drawPolarPattern(QPainter &painter, const QRect &rect)
{
    if (m_data.isEmpty()) return;

    // Определяем центр и максимальный радиус для отображения
    int centerX = rect.center().x();
    int centerY = rect.center().y();
    int radius = qMin(rect.width(), rect.height()) / 2 - 10; // 10px отступ от края

    // Рисуем вспомогательные окружности сетки через каждую четверть радиуса
    painter.setPen(QPen(Qt::lightGray, 1, Qt::DashLine));
    for (int r = radius / 4; r <= radius; r += radius / 4) {
        painter.drawEllipse(QPoint(centerX, centerY), r, r);
    }

    // Рисуем радиальные линии через каждые 30 градусов
    for (int angle = 0; angle < 360; angle += 30) {
        double radians = qDegreesToRadians(static_cast<double>(angle));
        int x = centerX + radius * cos(radians); // x = r * cos(θ)
        int y = centerY + radius * sin(radians); // y = r * sin(θ)
        painter.drawLine(centerX, centerY, x, y);

        // Добавляем метки углов за пределами основного круга
        int textX = centerX + (radius + 15) * cos(radians); // 15px отступ для текста
        int textY = centerY + (radius + 15) * sin(radians);
        painter.drawText(textX - 10, textY + 5, QString::number(angle) + "°");
    }

    // Получаем размеры данных диаграммы направленности
    int rowCount = m_data.size();
    int colCount = m_data[0].size(); // Количество углов в 360-градусной развертке

    QVector<QPointF> points;

    // Собираем точки для указанного среза (угол среза определяет индекс строки данных)
    for (int i = 0; i < colCount; ++i) {
        // Вычисляем угол для текущей точки (полный круг разбит на colCount частей)
        double angle = 360.0 * i / colCount;
        double radians = qDegreesToRadians(angle);

        // Берем данные из ряда, соответствующего выбранному срезу
        // Индекс строки ограничен размером массива данных
        int rowIndex = qMin(m_sliceAngle * rowCount / 360, rowCount - 1);

        // Нормализуем значение к диапазону [0,1]
        double value = normalizeValue(m_data[rowIndex][i]);

        // Масштабируем нормализованное значение до размера окна и преобразуем в декартовы координаты
        double r = value * radius;
        double x = centerX + r * cos(radians); // x = r * cos(θ)
        double y = centerY + r * sin(radians); // y = r * sin(θ)

        points.append(QPointF(x, y));
    }

    // Замыкаем контур, добавляя первую точку в конец массива
    if (!points.isEmpty()) {
        points.append(points.first());
    }

    // Рисуем контур диаграммы направленности
    painter.setPen(QPen(Qt::blue, 2));

    if (m_fillPattern) {
        // Заполняем диаграмму с полупрозрачным цветом, если включена заливка
        QColor fillColor = Qt::blue;
        fillColor.setAlphaF(0.3); // 30% непрозрачности
        painter.setBrush(fillColor);
    } else {
        painter.setBrush(Qt::NoBrush);
    }

    // Рисуем полигон, если есть хотя бы 2 точки
    if (points.size() > 1) {
        painter.drawPolygon(points.data(), points.size());
    }

    // Рисуем координатные оси
    painter.setPen(QPen(Qt::black, 2));
    painter.drawLine(centerX - radius, centerY, centerX + radius, centerY); // Ось X
    painter.drawLine(centerX, centerY - radius, centerX, centerY + radius); // Ось Y

    // Добавляем легенду с цветовой шкалой
    painter.drawText(rect.right() - 150, rect.top() + 20, "Нормализованное значение");
    for (int i = 0; i <= 10; ++i) {
        double val = i / 10.0; // 10 градаций от 0 до 1
        int y = rect.top() + 30 + i * 20;
        painter.fillRect(rect.right() - 150, y, 20, 15, getColorForValue(val));
        painter.drawRect(rect.right() - 150, y, 20, 15);
        painter.drawText(rect.right() - 120, y + 12, QString::number(val, 'f', 1));
    }
}

/**
 * Отрисовывает диаграмму направленности в 3D-проекции
 *
 * Математическая основа:
 * 1. Преобразование сферических координат в декартовы:
 *    x = r * sin(θ) * cos(φ)
 *    y = r * sin(θ) * sin(φ)
 *    z = r * cos(θ)
 *
 * 2. Матрица поворота вокруг оси Y (для угла места):
 *    |  cos(α)  0  sin(α) |
 *    |    0     1    0    |
 *    | -sin(α)  0  cos(α) |
 *
 * 3. Матрица поворота вокруг оси Z (для азимута):
 *    | cos(β)  -sin(β)  0 |
 *    | sin(β)   cos(β)  0 |
 *    |   0        0     1 |
 *
 * 4. Проецирование 3D-точек на 2D-плоскость экрана
 *
 * Где:
 *   - r - нормализованное значение (интенсивность)
 *   - θ - зенитный угол (отклонение от оси Z, 0-90°)
 *   - φ - азимутальный угол (0-360°)
 *   - α - угол места наблюдателя
 *   - β - азимутальный угол наблюдателя
 */
void PatternDiagramWindow::draw3DPattern(QPainter &painter, const QRect &rect)
{
    if (m_data.isEmpty()) return;

    // Определяем центр и размер области отображения
    int centerX = rect.center().x();
    int centerY = rect.center().y();
    int size = qMin(rect.width(), rect.height()) / 2 - 20; // 20px отступ от края

    // Сохраняем текущее состояние трансформации
    painter.save();
    // Перемещаем начало координат в центр области отображения
    painter.translate(centerX, centerY);

    // Рисуем оси координат в 3D с учетом текущего угла обзора
    painter.setPen(QPen(Qt::red, 2));
    painter.drawLine(0, 0, size, 0); // Ось X - красная
    painter.drawText(size + 5, 0, "X");

    painter.setPen(QPen(Qt::green, 2));
    painter.drawLine(0, 0, 0, -size); // Ось Y - зеленая
    painter.drawText(0, -size - 5, "Y");

    painter.setPen(QPen(Qt::blue, 2));
    // Вычисляем конечную точку оси Z с учетом текущего угла обзора
    QPointF zEnd = sphericalToCartesian(size, m_elevationAngle, m_azimuthAngle);
    painter.drawLine(0, 0, zEnd.x(), zEnd.y()); // Ось Z - синяя
    painter.drawText(zEnd.x() + 5, zEnd.y() + 5, "Z");

    // Рисуем вспомогательную сетку сферы (параллели и меридианы)
    painter.setPen(QPen(Qt::lightGray, 1, Qt::DashLine));

    // Горизонтальные круги (параллели) - линии постоянных зенитных углов θ
    for (int elevation = 0; elevation <= 90; elevation += 15) { // от 0° до 90° с шагом 15°
        QVector<QPointF> points;
        for (int azimuth = 0; azimuth < 360; azimuth += 5) { // Полный круг с малым шагом для гладкости
            points.append(sphericalToCartesian(size, elevation, azimuth));
        }
        points.append(points.first()); // Замыкаем контур

        if (points.size() > 1) {
            painter.drawPolyline(points.data(), points.size());
        }
    }

    // Вертикальные круги (меридианы) - линии постоянных азимутальных углов φ
    for (int azimuth = 0; azimuth < 360; azimuth += 30) { // от 0° до 360° с шагом 30°
        QVector<QPointF> points;
        for (int elevation = 0; elevation <= 90; elevation += 5) { // от 0° до 90° с малым шагом
            points.append(sphericalToCartesian(size, elevation, azimuth));
        }

        if (points.size() > 1) {
            painter.drawPolyline(points.data(), points.size());
        }
    }

    // Получаем размеры данных диаграммы направленности
    int rows = m_data.size(); // количество строк (зенитных углов θ)
    int cols = m_data[0].size(); // количество столбцов (азимутальных углов φ)

    painter.setPen(QPen(Qt::blue, 1));

    // Для каждого зенитного угла (θ) строим радиальную линию
    for (int i = 0; i < rows; ++i) {
        // Вычисляем угол возвышения (зенитный угол θ)
        double elevation = 90.0 * i / (rows - 1); // от 0° до 90°
        QVector<QPointF> points;

        // Для каждого азимутального угла (φ)
        for (int j = 0; j < cols; ++j) {
            // Вычисляем азимутальный угол
            double azimuth = 360.0 * j / cols; // от 0° до 360°

            // Нормализуем значение к диапазону [0,1]
            double value = normalizeValue(m_data[i][j]);

            // Преобразуем сферические координаты в декартовы с учетом текущего угла обзора
            points.append(sphericalToCartesian(value * size, elevation, azimuth));
        }

        // Замыкаем контур для каждого зенитного угла
        if (!points.isEmpty()) {
            points.append(points.first());
        }

        // Рисуем контур для текущего зенитного угла
        if (points.size() > 1) {
            if (m_fillPattern) {
                // Если включена заливка, используем полупрозрачный цвет
                QColor fillColor = getColorForValue(0.5); // средний цвет в спектре
                fillColor.setAlphaF(0.3); // 30% непрозрачности
                painter.setBrush(fillColor);
            } else {
                painter.setBrush(Qt::NoBrush);
            }

            // Для 3D в текущей реализации рисуем только контуры без заливки
            // из-за сложности определения перекрытий
            painter.drawPolyline(points.data(), points.size());
        }
    }

    // Восстанавливаем предыдущее состояние трансформации
    painter.restore();

    // Добавляем легенду с цветовой шкалой
    painter.drawText(rect.right() - 150, rect.top() + 20, "Нормализованное значение");
    for (int i = 0; i <= 10; ++i) {
        double val = i / 10.0; // 10 градаций от 0 до 1
        int y = rect.top() + 30 + i * 20;
        painter.fillRect(rect.right() - 150, y, 20, 15, getColorForValue(val));
        painter.drawRect(rect.right() - 150, y, 20, 15);
        painter.drawText(rect.right() - 120, y + 12, QString::number(val, 'f', 1));
    }
}

/**
 * Преобразует нормализованное значение в цвет для отображения интенсивности
 *
 * Использует HSV-модель цвета, где:
 * - Оттенок (Hue): изменяется от 240° (синий) до 0° (красный)
 * - Насыщенность (Saturation): максимальная (255)
 * - Яркость (Value): максимальная (255)
 *
 * Формула: hue = 240 - value * 240
 *
 * @param value нормализованное значение в диапазоне [0,1]
 * @return QColor цвет в спектре от синего (минимум) до красного (максимум)
 */
QColor PatternDiagramWindow::getColorForValue(double value) const
{
    // Преобразуем нормализованное значение [0,1] в угол оттенка [240,0]
    // где 240° - синий (для минимальных значений)
    // а 0° - красный (для максимальных значений)
    int hue = static_cast<int>(240 - value * 240);
    return QColor::fromHsv(hue, 255, 255); // Максимальная насыщенность и яркость
}

/**
 * Нормализует значение к диапазону [0,1] с учетом минимума и максимума данных
 * и выбранного типа шкалы (линейная или логарифмическая)
 *
 * Формулы нормализации:
 * - Линейная: norm_value = (value - min_value) / (max_value - min_value)
 * - Логарифмическая: norm_value = (ln(value) - ln(min_value)) / (ln(max_value) - ln(min_value))
 *
 * @param value исходное значение
 * @return нормализованное значение в диапазоне [0,1]
 */
double PatternDiagramWindow::normalizeValue(double value) const
{
    if (m_logarithmicScale) {
        // Для логарифмического масштаба защищаем от нулевых и отрицательных значений
        double minPositive = qMax(m_minValue, 1e-10); // Минимальное положительное значение
        double logMin = qLn(minPositive); // Натуральный логарифм минимума
        double logMax = qLn(qMax(m_maxValue, minPositive * 1.1)); // Натуральный логарифм максимума
        double logVal = qLn(qMax(value, minPositive)); // Натуральный логарифм значения

        // Нормализация в логарифмическом масштабе
        return (logVal - logMin) / (logMax - logMin);
    } else {
        // Для линейного масштаба
        return (value - m_minValue) / (m_maxValue - m_minValue);
    }
}

/**
 * Преобразует полярные координаты в декартовы
 *
 * Формулы:
 * x = radius * cos(angle)
 * y = radius * sin(angle)
 *
 * @param radius радиус (расстояние от начала координат)
 * @param angle угол в градусах (0° - вправо, 90° - вверх)
 * @return QPointF точка в декартовых координатах
 */
QPointF PatternDiagramWindow::polarToCartesian(double radius, double angle) const
{
    // Преобразование угла из градусов в радианы
    double radians = qDegreesToRadians(angle);
    // Применение формул преобразования полярных координат в декартовы
    return QPointF(radius * qCos(radians), radius * qSin(radians));
}

/**
 * Преобразует сферические координаты в декартовы с учетом текущего угла обзора
 *
 * Математический процесс:
 * 1. Преобразование из сферических в декартовы координаты:
 *    x = radius * sin(theta) * cos(phi)
 *    y = radius * sin(theta) * sin(phi)
 *    z = radius * cos(theta)
 *
 * 2. Применение поворота вокруг оси Y (для угла места):
 *    x' = x * cos(elevation) + z * sin(elevation)
 *    y' = y
 *    z' = -x * sin(elevation) + z * cos(elevation)
 *
 * 3. Применение поворота вокруг оси Z (для азимута):
 *    x'' = x' * cos(azimuth) - y' * sin(azimuth)
 *    y'' = x' * sin(azimuth) + y' * cos(azimuth)
 *    z'' = z'
 *
 * @param radius радиус (расстояние от начала координат)
 * @param theta зенитный угол в градусах (0° - вдоль оси Z, 90° - в плоскости XY)
 * @param phi азимутальный угол в градусах (0° - вдоль оси X, 90° - вдоль оси Y)
 * @return QPointF проекция 3D-точки на 2D-плоскость
 */
QPointF PatternDiagramWindow::sphericalToCartesian(double radius, double theta, double phi) const
{
    // theta - угол отклонения от оси Z (0-90 градусов)
    // phi - азимутальный угол в плоскости XY (0-360 градусов)
    double thetaRad = qDegreesToRadians(theta);
    double phiRad = qDegreesToRadians(phi);

    // Применяем поворот для угла обзора
    double azimuthRad = qDegreesToRadians(m_azimuthAngle);
    double elevationRad = qDegreesToRadians(m_elevationAngle);

    // Шаг 1: Преобразование из сферических в декартовы координаты
    // Стандартные формулы для сферических координат:
    // x = r * sin(θ) * cos(φ)
    // y = r * sin(θ) * sin(φ)
    // z = r * cos(θ)
    double x = radius * qSin(thetaRad) * qCos(phiRad);
    double y = radius * qSin(thetaRad) * qSin(phiRad);
    double z = radius * qCos(thetaRad);

    // Шаг 2: Применение матрицы поворота вокруг оси Y (для угла места)
    // [  cos(α)  0  sin(α)  ]   [ x ]   [ x*cos(α) + z*sin(α) ]
    // [    0     1     0    ] * [ y ] = [         y          ]
    // [ -sin(α)  0  cos(α)  ]   [ z ]   [ -x*sin(α) + z*cos(α)]
    double x2 = x * qCos(elevationRad) + z * qSin(elevationRad);
    double y2 = y;
    double z2 = -x * qSin(elevationRad) + z * qCos(elevationRad);

    // Шаг 3: Применение матрицы поворота вокруг оси Z (для азимута)
    // [ cos(β) -sin(β) 0 ]   [ x2 ]   [ x2*cos(β) - y2*sin(β) ]
    // [ sin(β)  cos(β) 0 ] * [ y2 ] = [ x2*sin(β) + y2*cos(β) ]
    // [   0       0    1 ]   [ z2 ]   [          z2          ]
    double x3 = x2 * qCos(azimuthRad) - y2 * qSin(azimuthRad);
    double y3 = x2 * qSin(azimuthRad) + y2 * qCos(azimuthRad);

    // Результат: проекция 3D точки на 2D экран
    // Инвертируем Y-координату для соответствия координатной системе экрана,
    // где ось Y направлена вниз, а не вверх
    return QPointF(x3, -y3);
}

/**
 * Обработка нажатия кнопки мыши
 *
 * Сохраняет начальную позицию мыши для расчета смещения при вращении вида
 * Обрабатывает только нажатия в области рисования диаграммы
 */
void PatternDiagramWindow::mousePressEvent(QMouseEvent *event)
{
    // Проверяем, находится ли курсор над областью рисования
    if (m_drawingArea && m_drawingArea->rect().contains(event->pos() - m_drawingArea->pos())) {
        m_lastMousePos = event->pos();
        event->accept();
    } else {
        // Если курсор не над областью рисования, передаем событие базовому классу
        QDialog::mousePressEvent(event);
    }
}

/**
 * Обработка перемещения мыши для вращения 3D-вида
 *
 * Математическая основа:
 * - Изменение азимутального угла пропорционально смещению по оси X
 * - Изменение угла места пропорционально смещению по оси Y (инвертированное)
 * - Ограничение угла места диапазоном [0°, 90°]
 *
 * Формулы:
 * azimuthAngle += deltaX * 0.5
 * elevationAngle = clamp(elevationAngle - deltaY * 0.5, 0, 90)
 */
void PatternDiagramWindow::mouseMoveEvent(QMouseEvent *event)
{
    // Обрабатываем только если нажата левая кнопка мыши и имеется сохраненная начальная позиция
    if (event->buttons() & Qt::LeftButton && !m_lastMousePos.isNull()) {
        // Вычисляем разницу между текущей и предыдущей позициями
        QPoint delta = event->pos() - m_lastMousePos;

        // Изменяем азимутальный угол пропорционально смещению по X
        // Коэффициент 0.5 подобран для комфортного вращения
        m_azimuthAngle += delta.x() * 0.5;

        // Изменяем угол места пропорционально смещению по Y (с инверсией)
        // Ограничиваем угол места диапазоном [0°, 90°]
        m_elevationAngle = qBound(0.0, m_elevationAngle - delta.y() * 0.5, 90.0);

        // Обновляем позицию для следующего вычисления разницы
        m_lastMousePos = event->pos();

        // Обновляем только область рисования для повышения производительности
        if (m_drawingArea) {
            m_drawingArea->update();
        }

        event->accept();
    } else {
        // Если не соблюдены условия для обработки, передаем событие базовому классу
        QDialog::mouseMoveEvent(event);
    }
}

/**
 * Обработка прокрутки колеса мыши для масштабирования
 *
 * Математическая основа:
 * - Изменение масштаба пропорционально углу поворота колеса
 * - Ограничение масштаба диапазоном [0.1, 10.0]
 *
 * Формула:
 * scale = clamp(scale * (1 + wheelDelta / 360), 0.1, 10.0)
 */
void PatternDiagramWindow::wheelEvent(QWheelEvent *event)
{
    // Проверяем, находится ли курсор над областью рисования
    if (m_drawingArea && m_drawingArea->rect().contains(event->position().toPoint() - m_drawingArea->pos())) {
        // Получаем угол поворота колеса мыши в градусах (деление на 8 - коэффициент Qt)
        QPoint numDegrees = event->angleDelta() / 8;
        if (!numDegrees.isNull()) {
            // Вычисляем коэффициент масштабирования
            // Для увеличения масштаба factor > 1, для уменьшения factor < 1
            double factor = 1.0 + numDegrees.y() / 360.0;

            // Применяем масштабирование с ограничением диапазона
            m_scale = qBound(0.1, m_scale * factor, 10.0);

            if (m_drawingArea) {
                m_drawingArea->update(); // Обновить только область рисования
            }

            event->accept();
        }
    } else {
        // Если курсор не над областью рисования, передаем событие базовому классу
        QDialog::wheelEvent(event);
    }
}

/**
 * Сохраняет текущее изображение диаграммы в PNG-файл
 *
 * Процесс:
 * 1. Создание пустого изображения размером с область рисования
 * 2. Рисование диаграммы в этом изображении с текущими параметрами
 * 3. Добавление информации о настройках
 * 4. Сохранение изображения в файл
 */
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

    // Добавляем информацию о настройках в верхний левый угол
    QString settings;
    settings += m_normalized ? "Нормализовано " : "";
    settings += m_logarithmicScale ? "Логарифмическая шкала " : "Линейная шкала ";
    settings += QString("Угол среза: %1°").arg(m_sliceAngle);

    painter.setPen(Qt::black);
    QFont font = painter.font();
    font.setPointSize(10);
    painter.setFont(font);
    painter.drawText(10, 20, settings);

    // Сохраняем изображение и выводим сообщение о результате
    if (pixmap.save(fileName)) {
        QMessageBox::information(this, tr("Успех"), tr("Изображение успешно сохранено."));
    } else {
        QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось сохранить изображение."));
    }
}

/**
 * Сбрасывает параметры отображения к значениям по умолчанию
 *
 * Сбрасываемые параметры:
 * - Масштаб (m_scale = 1.0)
 * - Азимутальный угол (m_azimuthAngle = 0.0)
 * - Угол места (m_elevationAngle = 30.0)
 * - Угол среза (m_sliceAngle = 0)
 */
void PatternDiagramWindow::resetView()
{
    // Сброс параметров отображения к значениям по умолчанию
    m_scale = 1.0;
    m_azimuthAngle = 0.0;
    m_elevationAngle = 30.0;
    m_sliceSlider->setValue(0);

    // Обновляем область рисования с новыми параметрами
    if (m_drawingArea) {
        m_drawingArea->update();
    }
}

/**
 * Переключение режима нормализации данных
 *
 * При нормализации данные масштабируются к диапазону [0,1]
 * в соответствии с минимальным и максимальным значениями
 *
 * @param normalized флаг включения нормализации
 */
void PatternDiagramWindow::toggleNormalization(bool normalized)
{
    m_normalized = normalized;
    if (m_drawingArea) {
        m_drawingArea->update();
    }
}

/**
 * Переключение между линейной и логарифмической шкалой
 *
 * Логарифмическая шкала используется для лучшего отображения данных
 * с большим динамическим диапазоном, где мелкие детали могут быть
 * незаметны в линейном масштабе
 *
 * @param useLog флаг использования логарифмической шкалы
 */
void PatternDiagramWindow::toggleLogScale(bool useLog)
{
    m_logarithmicScale = useLog;
    if (m_drawingArea) {
        m_drawingArea->update();
    }
}

/**
 * Переключение режима заливки диаграммы
 *
 * При включенной заливке диаграмма отображается как закрашенная область,
 * при выключенной - только как контур
 *
 * @param fill флаг включения заливки
 */
void PatternDiagramWindow::toggleFillMode(bool fill)
{
    m_fillPattern = fill;
    if (m_drawingArea) {
        m_drawingArea->update();
    }
}

/**
 * Изменение типа проекции диаграммы (2D или 3D)
 *
 * @param index индекс типа проекции (0 - полярная 2D, 1 - сферическая 3D)
 */
void PatternDiagramWindow::updateProjectionType(int index)
{
    m_projectionType = index;
    if (m_drawingArea) {
        m_drawingArea->update();
    }
}

/**
 * Обновление угла среза для 2D-диаграммы
 *
 * В 2D-представлении мы показываем срез 3D-данных под определенным углом.
 * Этот метод обновляет угол среза и текстовую метку.
 *
 * @param angle угол среза в градусах (0-360)
 */
void PatternDiagramWindow::updateSliceAngle(int angle)
{
    m_sliceAngle = angle;
    m_sliceLabel->setText(QString("Угол среза: %1°").arg(angle));
    if (m_drawingArea) {
        m_drawingArea->update();
    }
}

/**
 * Устанавливает азимутальный угол обзора
 *
 * @param angle угол в градусах
 */
void PatternDiagramWindow::setAzimuthAngle(double angle)
{
    m_azimuthAngle = angle;
    if (m_drawingArea) {
        m_drawingArea->update();
    }
}

/**
 * Устанавливает угол места (возвышения) для обзора
 * с ограничением в диапазоне [0, 90] градусов
 *
 * @param angle угол в градусах
 */
void PatternDiagramWindow::setElevationAngle(double angle)
{
    m_elevationAngle = qBound(0.0, angle, 90.0);
    if (m_drawingArea) {
        m_drawingArea->update();
    }
}
