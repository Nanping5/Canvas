#ifndef ANIMATIONWINDOW_H
#define ANIMATIONWINDOW_H

#include <QWidget>
#include <QPushButton>

class AnimationWindow : public QWidget {
    Q_OBJECT
public:
    explicit AnimationWindow(QWidget *parent = nullptr);
    
private:
    QPushButton *backButton;
    
signals:
    void backToMain();
};

#endif // ANIMATIONWINDOW_H 