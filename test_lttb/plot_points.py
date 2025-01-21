import csv
import matplotlib.pyplot as plt
 
x1 = []
y1 = []
 
x2 = []
y2 = []
 
# 读取csv文件
with open('point_data.csv', 'r') as file:
    reader = csv.reader(file)
    for row in reader:
        x1.append(float(row[0]))
        y1.append(float(row[1]))
 
with open('fitting_result.csv', 'r') as file:
    reader = csv.reader(file)
    for row in reader:
        x2.append(float(row[0]))
        y2.append(float(row[1]))

# 绘制折线图
plt.plot(x1, y1, label='point data')
plt.plot(x2, y2, label='fittingresult')
 
# 设置横轴和纵轴标签
plt.xlabel('X')
plt.ylabel('Y')

# 显示图例
plt.legend() 

# 显示图表
plt.show()

