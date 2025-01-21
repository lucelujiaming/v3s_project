// test_lttb.cpp : 定义控制台应用程序的入口点。
//
#ifdef _WIN32
#include "stdafx.h"
#pragma warning(disable:4996)
#else
#include <memory.h>
#include <string.h>
#endif

#include <iostream>
#include <vector>
using namespace std;

#define MAX_LINE_LENGTH 1024

typedef struct PointData_ {
	double X;
	double Y;
} PointData;

bool get_point_data(char * cFileName, vector<PointData>& vectorPointData)
{
	FILE *file = fopen(cFileName, "r"); // 打开文件
	if (file == NULL) {
		perror("Error opening file");
		return false;
	}

	char line[MAX_LINE_LENGTH];
	char cXContent[10];
	char cYContent[10];
	while (fgets(line, sizeof(line), file) != NULL) {
		// 处理读取到的每一行
		// printf("%s", line);
		char* cSeq = strchr(line, ',');
		if (cSeq) {
			memset(cXContent, 0X00, 10);
			memset(cYContent, 0X00, 10);
			memcpy(cXContent, line, cSeq - line);
			memcpy(cYContent, cSeq + 1, strlen(line) - (cSeq + 1 - line));
			PointData pointData;
			pointData.X = atof(cXContent);
			pointData.Y = atof(cYContent);
			vectorPointData.push_back(pointData);
		}
	}
	fclose(file); // 关闭文件
	return true;
}

/*
* LTTB算法由冰岛大学的Sveinn在2013年提出，其对时序数据的降维拟合效果显著。
* 什么叫数据拟合呢，说人话就是，用大概不差的线拟合之前的数据。
* 这个算法的逻辑是这样的。
* 输入两个参数。第一个参数是一个点列表。第二个参数是一个拟合以后的点个数。
* 首先
* 计算逻辑如下：
* 1. 把起始点加入拟合结果。因为我们准备拟合的点应该是以起始点开始。
*    这里定义为点A。
* 2. 之后我们需要根据输入的拟合以后的点个数，计算点列表中每一个拟合组的大小。
*    计算公式为：
*        拟合组大小 = (点个数 - 2) / (拟合点个数 - 2)
*    点个数减2和点个数减2的原因是除去首尾。因为起始点和结束点自成一拟合组。
* 之后我们一个拟合组一个拟合组计算，计算每一个拟合组里面的拟合值。
* 3. 我们首先计算这个拟合组里面的平均点。
*    方法就是把拟合组里面所有的X求和除以点个数。所有的Y求和除以点个数。
*    这里定义为点C。
* 4. 遍历这个拟合组里面的每一个点。这里定义为点B。
*    我们逐个计算每一个点B和点A，还有点C组成的三角形△ABC的面积。
*    
* 5. 在找到面积最大的那个三角形的时候，
*    这个三角形对应的点B就是我们选中的拟合点。
*    同时我们需要记下这个选中点的索引。
*    因为这个选中点在下一拟合组中会作为点A，进行三角形的面积运算。
* 
*    这个就是这个算法中最巧妙的部分。
*    就是我们首先让起始点做点A，之后在第一组中计算出来平均点作为点C。
*    之后通过查找最大三角形面积得到第一组中的选中拟合点，这里叫点B。
*    而这个选中的拟合点B，又会作为第二组中用于计算最大三角形面积的点A。
*    以此类推，直到最后一组。之后加上尾点，就得到我们需要的拟合结果。
* 
*    这样选取的点，一方面就是属于原有数据中的点，一方面还兼顾了平均值和数据的起伏。
* 
* 6. 添加尾点。因为结束点自成一组。
*/
static vector<PointData> lttb_fitting(vector<PointData> data, int threshold)
{
	int dataLength = data.size();
	if (threshold >= dataLength || threshold == 0)
		return data; // Nothing to do

	vector<PointData> sampled;
	sampled.reserve(threshold);

	// 1. 初始点A为首点。
	int iCurrentPointAIndex = 0;
    // 添加首点，因为起始点自成一组。
	sampled.push_back(data[iCurrentPointAIndex]); 
    // 记录最大面积对应点。
	PointData maxAreaPoint = { 0.0, 0.0 };
    // 记录最大面积对应点的索引。
	int iNextPointAIndex = 0;

	/*
	 * 2. 计算比值（除去首尾，每个点集的数量）
	 * 点集大小，排除首尾点，阈值减2，
	 */
	double dGroupSize = (double)(dataLength - 2) / (threshold - 2);

	for (int i = 0; i < threshold - 2; i++)
	{
		// 3. 计算点集3中点的平均点
		// 假设第三个点是此前区间的平均值，大概率不存在，凭空捏造一个
		PointData avgPoint = { 0.0, 0.0 };
		// 3.1 计算当前点集组的起始索引。
		int avgRangeStart = (int)(((i + 1) * dGroupSize) + 1);
		// 3.2 计算当前点集组的起始索引。
		int avgRangeEnd = (int)(((i + 2) * dGroupSize) + 1);
		avgRangeEnd = avgRangeEnd < dataLength ? avgRangeEnd : dataLength;
		// 3.3 计算当前点集组的长度。
		int avgRangeLength = avgRangeEnd - avgRangeStart;
		// 3.4 计算当前点集组的坐标和。
		for (; avgRangeStart < avgRangeEnd; avgRangeStart++)
		{
			avgPoint.X += data[avgRangeStart].X; // * 1 enforces Number (value may be Date)
			avgPoint.Y += data[avgRangeStart].Y;
		}
		// 3.5 当前点集组的坐标和除以点集组的长度得到平均值。
		avgPoint.X /= avgRangeLength;
		avgPoint.Y /= avgRangeLength;

		// +1是为了排除当前点
		int rangeOffs = (int)((int)((i + 0) * dGroupSize) + 1);
		int rangeTo = (int)((int)((i + 1) * dGroupSize) + 1);

		// 4 找到点集B，点C和点A形成最大的三角形面积，
		//   这个最大的三角形中的点B就是我们要找的点。
		// 4.1 首先记录下点A的位置。
		PointData pointA = data[iCurrentPointAIndex];
        // 4.2 用于记录点集B，点C和点A形成最大的三角形面积的变量。
		double maxArea = -1;
		// 4.3 一个一个计算点集B，点C和点A形成的三角形面积，从而找到最大的。
		for (; rangeOffs < rangeTo; rangeOffs++)
		{
            /*****************************************************************
			 * 计算面积：(点A, 点集B, 虚拟的平均值点C)
             *****************************************************************
             * 通过查询百度百科。可以知道。
             * 以△ABC的一个顶点A为原点,建立平面直角坐标系.
             * 分别记B和C点的坐标为(x1, y1)和(x2, y2)。
             * 则通过简单的初等几何关系，可以得知三角形面积可以表达为：
             *   S = | x2 * y1 - 1/2 * x2 * y2 - 1/2 * x1 * y1 - 1/2 * (x2 - x1) * (y1 - y2) |
             *     = |x1 * y2 - x2 * y1|
             * 显然，若A不在原点，则将相关坐标替换为坐标差即可。
             * 这就是下面的代码使用的计算公式。
             *****************************************************************
             * 顺便可知，此时,有：
             *        |       |xA, xA, 1| |
             *    S = | 1/2 * |xB, xB, 1| |
             *        |       |xC, xC, 1| |
             * 这就是三角形面积的行列式形式(解析几何)
             *****************************************************************/
			double area = abs((pointA.X - avgPoint.X) * (data[rangeOffs].Y - pointA.Y) -
				              (pointA.X - data[rangeOffs].X) * (avgPoint.Y - pointA.Y)
				             ) * 0.5;
			if (area > maxArea)
			{
				maxArea = area;
				maxAreaPoint = data[rangeOffs];
				iNextPointAIndex = rangeOffs; // 设置下一个点
			}
		}
		// 5.1  循环结束以后，我们找到了最大面积对应的点，我们把这个点加入输出点集。
		sampled.push_back(maxAreaPoint);
		// 5.2 而这个最大面积对应的点B。就是用于寻找下一组中最大面积点用的点A。
        //     因此上这里记下这个点对应的索引。
		iCurrentPointAIndex = iNextPointAIndex; 
	}
	// 6. 添加尾点。
	sampled.push_back(data[dataLength - 1]);
	return sampled;
}

bool save_fitting_result(char * cFileName, vector<PointData> vectorResult)
{
	FILE *file = fopen(cFileName, "w"); // 打开文件
	if (file == NULL) {
		perror("Error opening file");
		return false;
	}
	char line[MAX_LINE_LENGTH];
	for (auto it = vectorResult.begin(); it != vectorResult.end(); ++it) {
		sprintf(line, "%.2f,%.2f\n", it->X, it->Y);
		fputs(line, file);
	}

	fclose(file); // 关闭文件
	return true;

}


int main(int argc, char* argv[])
{
	vector<PointData> vectorPointData;
	get_point_data((char *)"point_data.csv", vectorPointData);
	std::cout << "vectorPointData has " << vectorPointData.size() << " elements " << std::endl;
	// // 使用迭代器打印vector
	// for (auto it = vectorPointData.begin(); it != vectorPointData.end(); ++it) {
	// 	std::cout << "X = " << it->X << " Y = " << it->Y << std::endl;
	// }
	vector<PointData> vectorResult = lttb_fitting(vectorPointData, 50);
	std::cout << "vectorResult has " << vectorResult.size() << " elements " << std::endl;
	// // 使用迭代器打印vector
	// for (auto it = vectorResult.begin(); it != vectorResult.end(); ++it) {
	// 	std::cout << "X = " << it->X << " Y = " << it->Y << std::endl;
	// }
	save_fitting_result((char *)"fitting_result.csv", vectorResult);
	return 0;
}


