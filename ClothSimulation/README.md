https://www.cs.rpi.edu/~cutler/classes/advancedgraphics/S13/hw2_simulation.html

实验步骤:
### 1.显式欧拉法

一阶显式欧拉法在0.001步长下能够处理至少5x5大小的small_cloth，但将small_cloth的大小调整至5x6、6x6时，就无法实现正常的效果。
  
<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-itfHww.png' width="50%" >

### 2.实现Trapezoid Method（梯形法/二阶法）

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-qBnvWp.png' width="30%" >

结果:

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-AqlCfH.png' width="70%" >

### 3.添加规则限制变形率

根据下图的规则调整拉伸超过变形率spring的长度和速度

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-zr3FfA.png' width="50%" >

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-pNoaG7.png' width="100%" >


<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-h1jx3F.png' width="100%" >

为了模拟真实的效果，不应当只考虑布料过度拉伸的情况，应该同时考虑布料被过度压缩的情况： 

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-0adTIG.png' width="100%" >

缺陷：添加了变形率限制的Trapezoid在处理denim_curtain时会出现如下图的问题：

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-jQODWk.png' width="100%" >
  
### 4.自适应步长
初始时可以使用较大的步长，当发现不稳定时，将步长减小，重做当前这一步模拟。自适应步长能比固定步长方法更快地到达稳定位置。在调整步长时应当添加上下限来进行限制，避免出现步长极小导致布料几乎不动的情况和步长极大导致布料直接消失不见。但自适应步长的问题是必须针对处理的布料提前设置好判定用的超参数。如果这个判定不稳定的值选取的不合适，会导致自适应步长比变步长花费更多的时间在调整步长上。
不稳定判定条件：布料在某一时刻不稳定的条件是当前步所有粒子的最大速度超过上一步所有粒子的最大速度一定值。这个值是提前设好的超参数。

以provot_correct_structural_and_shear为例，固定步长+Trapezoid以0.01步长执行，需要大概3000次Animate才能到达稳定状态，而采用自适应步长，粗略调整参数后（不是最优的），大概需要2000次Animate就能到达稳定状态。
原本0.01固定步长下一阶欧拉法无法成功实现provot_correct_structural_and_shear，而采用自适应步长能让步长调整到更小的合适的步长，从而实现正常的模拟
  
<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-hb3ZIw.png' width="100%" >

### 5.(Extra)Midpoint Method（中点法）

利用当前状态的1/2步长之后的状态来更新当前状态，相比于Trapezoid法，能够更快地对变化做出反应。

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-giilwU.png' width="30%" >

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-eZ7Eww.png' width="100%" >

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-TKUUMH.png' width="100%" >

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-oxI6Uj.png' width="100%" >

与Trapezoid法相比，Midpoint能够处理denim_curtain的情况，在一些情况下模拟出的布料效果更加真实。Midpoint能比Trapezoid更早到达稳定位置且出现抖动的情况更少。
