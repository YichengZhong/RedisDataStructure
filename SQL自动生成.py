# -*- coding: utf-8 -*-
'''
多窗口反复切换，只用PyQt5实现
'''
import sys  # 导入系统
from PyQt5.QtWidgets import QApplication, QMainWindow, QWidget, QPushButton
from PyQt5.QtWidgets import (QWidget, QLabel,QLineEdit, QApplication,QPushButton,QTextEdit)
import json

class FirstUi(QMainWindow):  # 第一个窗口类
    '''
    确定表名，还有几个参数
    '''
    def __init__(self):
        super(FirstUi, self).__init__()
        self.init_ui()

    def init_ui(self):
        self.resize(300, 200)  # 设置窗口大小
        self.setWindowTitle('获取表名以及参数个数')  # 设置窗口标题

        self.lb_dealname = QLabel('自动生成SQL语句Demo', self)
        self.lb_dealname.setGeometry(10, 0, 120, 30)

        self.lb_tablename = QLabel('表名：', self)
        self.le_tablename = QLineEdit(self)
        self.lb_tablename.setGeometry(10, 30, 60, 30)
        self.le_tablename.setGeometry(65, 35, 50, 20)

        self.lb_paranum = QLabel('参数个数：', self)
        self.le_paranum = QLineEdit(self)
        self.lb_paranum.setGeometry(10, 60, 60, 40)
        self.le_paranum.setGeometry(65, 70, 50, 20)

        self.btn_Next = QPushButton('下一步', self)  # 设置按钮和按钮名称
        self.btn_Next.setGeometry(50, 100, 100, 50)  # 前面是按钮左上角坐标，后面是窗口大小
        self.btn_Next.clicked.connect(self.slot_btn_function)  # 将信号连接到槽

    def slot_btn_function(self):
        if(self.le_tablename.text()=="" or (not self.le_paranum.text().isdigit()) ):
            print("不合格")
            return

        with open('tableName_ParaNums.json', 'w',encoding='utf-8') as f:
            str={'TableName':self.le_tablename.text(),'ParaNums':self.le_paranum.text()}

            json.dump(str, f)

        self.hide()  # 隐藏此窗口
        self.s = SecondUi()  # 将第二个窗口换个名字
        self.s.show()  # 经第二个窗口显示出来


class SecondUi(QWidget):  # 建立第二个窗口的类
    '''
    确定表中每个参数的名字
    '''
    def __init__(self):
        super(SecondUi, self).__init__()
        self.init_ui()

    def init_ui(self):
        self.TableName = ""
        self.ParaNums=0
        self.High=200
        #获取参数个数，决定窗口大小
        with open('tableName_ParaNums.json', encoding='utf-8') as f:  # 打开‘product.json’的json文件
            res = f.read()  # 读文件
            table_data=json.loads(res)
            print(table_data)  # 把json串变成python的数据类型：字典
            self.TableName = table_data['TableName']
            self.ParaNums=int(table_data['ParaNums'])

        if(self.High<(60+30*self.ParaNums)):
            self.High=60+30*self.ParaNums

        self.resize(400, self.High)  # 设置第二个窗口代码
        self.setWindowTitle('获取各个参数名字')  # 设置第二个窗口标题

        self.btn_Pre = QPushButton('上一步', self)  # 设置按钮和按钮名称
        self.btn_Pre.setGeometry(150, 150, 100, 50)  # 前面是按钮左上角坐标，后面是按钮大小
        self.btn_Pre.clicked.connect(self.slot_btn_function_pre)  # 将信号连接到槽

        self.btn_Next = QPushButton('下一步', self)  # 设置按钮和按钮名称
        self.btn_Next.setGeometry(250, 150, 100, 50)  # 前面是按钮左上角坐标，后面是按钮大小
        self.btn_Next.clicked.connect(self.slot_btn_function_next)  # 将信号连接到槽

    def slot_btn_function_pre(self):
        self.hide()  # 隐藏此窗口
        self.f = FirstUi()  # 将上一个窗口换个名字
        self.f.show()  # 将上一个窗口显示出来

    def slot_btn_function_next(self):
        self.hide()  # 隐藏此窗口
        self.f = ThirdUi()  # 将第三个窗口换个名字
        self.f.show()  # 将第三个窗口显示出来


class ThirdUi(QWidget):  # 建立第三个窗口的类
    def __init__(self):
        super(ThirdUi, self).__init__()
        self.init_ui()

    def init_ui(self):
        self.resize(500, 350)  # 设置第二个窗口代码
        self.setWindowTitle('Third Ui')  # 设置第二个窗口标题
        self.btn = QPushButton('jump3to1', self)  # 设置按钮和按钮名称
        self.btn.setGeometry(150, 150, 100, 50)  # 前面是按钮左上角坐标，后面是按钮大小
        self.btn.clicked.connect(self.slot_btn_function)  # 将信号连接到槽

    def slot_btn_function(self):
        self.hide()  # 隐藏此窗口
        self.f = FirstUi()  # 将第一个窗口换个名字
        self.f.show()  # 将第一个窗口显示出来


def main():
    app = QApplication(sys.argv)
    w = FirstUi()  # 将第一和窗口换个名字
    w.show()  # 将第一和窗口换个名字显示出来
    sys.exit(app.exec_())  # app.exet_()是指程序一直循环运行直到主窗口被关闭终止进程（如果没有这句话，程序运行时会一闪而过）


if __name__ == '__main__':  # 只有在本py文件中才能用，被调用就不执行
    main()
