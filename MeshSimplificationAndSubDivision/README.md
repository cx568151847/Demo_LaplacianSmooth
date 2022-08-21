# Mesh Simplification And SubDivision

https://www.cs.rpi.edu/~cutler/classes/advancedgraphics/S13/hw1_meshes.html

## 1.Simplification_Random，随机选择边删除.

思路：随机从网格中选择一条边进行删除，同时删除这条边所在的两个三角形，同时使用删除边的两个顶点计算出一个新的顶点(两点取平均），该点与周围的点构建新的三角形。从而实现对整个网格的简化。
遇到的问题：

(1)	并非所有的边都可以删除,如果一条半边没有其对应的opposite，那么这条边在边界上，不能删除。

(2)	如果一条边其周围的点构成的环并不唯一，那么也不能删除。例如删除边ab后，新的顶点是N，如果1和4是同一个点，删除后需要添加1N，4N，则会出现重复创建边的情况。对于环上其他的点也是如此。因此加一个判断，如果1234567构成的环里面还有环，假设1和4指向同一点，那么还包含环1234,4567。不能删除边ab。

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-auKc0D.png' width="30%" >

(3)	新顶点是取两点的平均求得的，可能会导致新生成的三角形面翻转，将内部(蓝色部分)暴露在外面。之后使用QEM选择新的顶点可以解决这个问题。
效果:

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-ffKqCI.png' width="100%" >


## 2.Simplification_QEM,使用QEM选择边删除
思路:计算出每一个顶点的Q，计算出每一条边的误差。使用一个小顶堆来将边按误差从小到大的顺序排列。每次删除时从堆顶得到误差最小的边开始删除。每条边删除后需要更新小顶堆中相关的边，删除已经不存在的边并为相关的边重新计算误差。
(1)删除边后需要处理的有3种边：

a.被删除的边AB及其半边BA

b.其中一端为A或B的边(BC)，这些边在removeTriangle中被删除了，需要将对应的新边(CN)添加到小顶堆中。

c.两个端点既不是A也不是B的边(CD)，但其中一个端点C与A或B相连，三角形改变导致点C的误差值需要重新计算，因此CD的误差也需要重新计算。

(2)计算最优坐标时的矩阵并不总是可逆的。当矩阵不可逆时，在欲删除的边AB上(尽量)找一个误差最小的点。

(3)与随机选择边删除时相同，边界边和周围点构成的环不唯一的边不能删除。对于这些不能删除的边，将其误差设置成一个足够大的数，确保选择边进行删除时不会选择到它们。
效果:
    
<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-1GaAn2.png' width="100%" >

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-SbgNgH.png' width="100%" >

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-hZ9Rf2.png' width="100%" >

## 3.Loop Subdivision 

crease是在obj文件中使用e [value1] [value2] [value3]格式标注的边，在提供的绘制代码运行时被绘制为黄色。引入crease边是为了在简化时能够保留一些特定的部分，比如棱角。根据Piecewise Smooth Surface Reconstruction中描述的方法，将每个点按照crease边的入度s分别划分为normal vertex、dart vertex、crease vertex和corner vertex，其中crease vertex根据边的数量以及排列分为regular crease vertex和non-regular crease vertex.
在创建一条边上的新顶点odd时根据下图的Table.1来从3种不同的权重的分配方法中选择出一种来计算出新顶点的坐标。

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-G0BiWK.png' width="50%" >

更新旧顶点even时，根据顶点类型来选择不同的权重分配方法来计算新的坐标。

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-B8UvVp.png' width="50%" >

效果:
    
<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-sAuDVz.png' width="100%" >

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-FhLGmq.png' width="100%" >

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-Zc66mm.png' width="100%" >

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-5oKVFT.png' width="100%" >

## 4.Modified Butterfly Subdivision 

Modified Butterfly Subdivision计算新顶点的方式不同，而且不需要更新旧顶点的坐标。计算新顶点坐标时需要根据两个顶点相连边的数量不同分别采取不同的权重分配。
应该注意的问题：

(1)	在寻找一条非边界边端点的周围点时，如果遇到边界边。向另一个方向寻找另外一条边界边，并且，因为在分配权重时与可能周围点的顺序有关，所以应当注意存储周围顶点的顺序问题。

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-OmVBca.png' width="30%" >

(2)在处理crease边时，如果一个顶点周围有多条crease边，在计算时让着多条crease边的另一个端点平分-1/16的权重。
 
<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-9UnjJw.png' width="30%" >

(3)同样是在处理crease边时，如果一个顶点周围除了当前正在处理的边以外一条crease边都没有，那么当前顶点的权重直接设为1/2。
效果:
    
<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-d8hY7h.png' width="100%" >

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-nfFkug.png' width="100%" >

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-ECus4k.png' width="100%" >

# 分析

一些我自己的想法，不一定对
##Loop Subdivision与Modified Butterfly Subdivision的比较
### (1)	对于封闭的模型:
可以发现Loop Subdivision的处理结果更加圆滑，而Modified Butterfly Subdivision的结果更加起伏不平。
   
<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-JdaPGU.png' width="100%" >

### (2)对于不含crease边的不封闭模型:

Modified Butterfly Subdivisiond细分的结果并没有Loop Subdivision细分的结果规则，反而像是凭空增加了起伏。
对于边界的处理，Modified Butterfly Subdivisiond方法采用了下图所示的权重分配策略来计算新的顶点：

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-FEaaha.png' width="30%" >

而且Modified Butterfly Subdivisiond方法并不更新旧顶点的坐标，所以会稍微放大网格上不封闭的部分。
        
<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-R2CCgy.png' width="100%" >

### (3)	对于含crease边的不封闭模型: 

Loop Subdivision处理的效果更好，保留crease边的同时细分了其他部分。而Modified Butterfly Subdivision处理的结果依然是凹凸不平，在处理crease边时还会裂开，露出蓝色的内部区域。对于对称的、不封闭的模型，Loop Subdivision更加合适。
  
<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-cBB8jC.png' width="100%" >

