# Mesh Simplification And SubDivision

https://www.cs.rpi.edu/~cutler/classes/advancedgraphics/S13/hw1_meshes.html

## 1.Simplification_Random�����ѡ���ɾ��.

˼·�������������ѡ��һ���߽���ɾ����ͬʱɾ�����������ڵ����������Σ�ͬʱʹ��ɾ���ߵ�������������һ���µĶ���(����ȡƽ�������õ�����Χ�ĵ㹹���µ������Ρ��Ӷ�ʵ�ֶ���������ļ򻯡�
���������⣺

(1)	�������еı߶�����ɾ��,���һ�����û�����Ӧ��opposite����ô�������ڱ߽��ϣ�����ɾ����

(2)	���һ��������Χ�ĵ㹹�ɵĻ�����Ψһ����ôҲ����ɾ��������ɾ����ab���µĶ�����N�����1��4��ͬһ���㣬ɾ������Ҫ���1N��4N���������ظ������ߵ���������ڻ��������ĵ�Ҳ����ˡ���˼�һ���жϣ����1234567���ɵĻ����滹�л�������1��4ָ��ͬһ�㣬��ô��������1234,4567������ɾ����ab��

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-auKc0D.png' width="30%" >

(3)	�¶�����ȡ�����ƽ����õģ����ܻᵼ�������ɵ��������淭ת�����ڲ�(��ɫ����)��¶�����档֮��ʹ��QEMѡ���µĶ�����Խ��������⡣
Ч��:

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-ffKqCI.png' width="100%" >


## 2.Simplification_QEM,ʹ��QEMѡ���ɾ��
˼·:�����ÿһ�������Q�������ÿһ���ߵ���ʹ��һ��С���������߰�����С�����˳�����С�ÿ��ɾ��ʱ�ӶѶ��õ������С�ı߿�ʼɾ����ÿ����ɾ������Ҫ����С��������صıߣ�ɾ���Ѿ������ڵı߲�Ϊ��صı����¼�����
(1)ɾ���ߺ���Ҫ�������3�ֱߣ�

a.��ɾ���ı�AB������BA

b.����һ��ΪA��B�ı�(BC)����Щ����removeTriangle�б�ɾ���ˣ���Ҫ����Ӧ���±�(CN)��ӵ�С�����С�

c.�����˵�Ȳ���AҲ����B�ı�(CD)��������һ���˵�C��A��B�����������θı䵼�µ�C�����ֵ��Ҫ���¼��㣬���CD�����Ҳ��Ҫ���¼��㡣

(2)������������ʱ�ľ��󲢲����ǿ���ġ������󲻿���ʱ������ɾ���ı�AB��(����)��һ�������С�ĵ㡣

(3)�����ѡ���ɾ��ʱ��ͬ���߽�ߺ���Χ�㹹�ɵĻ���Ψһ�ı߲���ɾ����������Щ����ɾ���ıߣ�����������ó�һ���㹻�������ȷ��ѡ��߽���ɾ��ʱ����ѡ�����ǡ�
Ч��:
    
<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-1GaAn2.png' width="100%" >

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-SbgNgH.png' width="100%" >

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-hZ9Rf2.png' width="100%" >

## 3.Loop Subdivision 

crease����obj�ļ���ʹ��e [value1] [value2] [value3]��ʽ��ע�ıߣ����ṩ�Ļ��ƴ�������ʱ������Ϊ��ɫ������crease����Ϊ���ڼ�ʱ�ܹ�����һЩ�ض��Ĳ��֣�������ǡ�����Piecewise Smooth Surface Reconstruction�������ķ�������ÿ���㰴��crease�ߵ����s�ֱ𻮷�Ϊnormal vertex��dart vertex��crease vertex��corner vertex������crease vertex���ݱߵ������Լ����з�Ϊregular crease vertex��non-regular crease vertex.
�ڴ���һ�����ϵ��¶���oddʱ������ͼ��Table.1����3�ֲ�ͬ��Ȩ�صķ��䷽����ѡ���һ����������¶�������ꡣ

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-G0BiWK.png' width="50%" >

���¾ɶ���evenʱ�����ݶ���������ѡ��ͬ��Ȩ�ط��䷽���������µ����ꡣ

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-B8UvVp.png' width="50%" >

Ч��:
    
<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-sAuDVz.png' width="100%" >

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-FhLGmq.png' width="100%" >

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-Zc66mm.png' width="100%" >

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-5oKVFT.png' width="100%" >

## 4.Modified Butterfly Subdivision 

Modified Butterfly Subdivision�����¶���ķ�ʽ��ͬ�����Ҳ���Ҫ���¾ɶ�������ꡣ�����¶�������ʱ��Ҫ�����������������ߵ�������ͬ�ֱ��ȡ��ͬ��Ȩ�ط��䡣
Ӧ��ע������⣺

(1)	��Ѱ��һ���Ǳ߽�߶˵����Χ��ʱ����������߽�ߡ�����һ������Ѱ������һ���߽�ߣ����ң���Ϊ�ڷ���Ȩ��ʱ�������Χ���˳���йأ�����Ӧ��ע��洢��Χ�����˳�����⡣

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-OmVBca.png' width="30%" >

(2)�ڴ���crease��ʱ�����һ��������Χ�ж���crease�ߣ��ڼ���ʱ���Ŷ���crease�ߵ���һ���˵�ƽ��-1/16��Ȩ�ء�
 
<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-9UnjJw.png' width="30%" >

(3)ͬ�����ڴ���crease��ʱ�����һ��������Χ���˵�ǰ���ڴ���ı�����һ��crease�߶�û�У���ô��ǰ�����Ȩ��ֱ����Ϊ1/2��
Ч��:
    
<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-d8hY7h.png' width="100%" >

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-nfFkug.png' width="100%" >

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-ECus4k.png' width="100%" >

# ����

һЩ���Լ����뷨����һ����
##Loop Subdivision��Modified Butterfly Subdivision�ıȽ�
### (1)	���ڷ�յ�ģ��:
���Է���Loop Subdivision�Ĵ���������Բ������Modified Butterfly Subdivision�Ľ�����������ƽ��
   
<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-JdaPGU.png' width="100%" >

### (2)���ڲ���crease�ߵĲ����ģ��:

Modified Butterfly Subdivisiondϸ�ֵĽ����û��Loop Subdivisionϸ�ֵĽ�����򣬷�������ƾ�������������
���ڱ߽�Ĵ���Modified Butterfly Subdivisiond������������ͼ��ʾ��Ȩ�ط�������������µĶ��㣺

<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-FEaaha.png' width="30%" >

����Modified Butterfly Subdivisiond�����������¾ɶ�������꣬���Ի���΢�Ŵ������ϲ���յĲ��֡�
        
<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-R2CCgy.png' width="100%" >

### (3)	���ں�crease�ߵĲ����ģ��: 

Loop Subdivision�����Ч�����ã�����crease�ߵ�ͬʱϸ�����������֡���Modified Butterfly Subdivision����Ľ����Ȼ�ǰ�͹��ƽ���ڴ���crease��ʱ�����ѿ���¶����ɫ���ڲ����򡣶��ڶԳƵġ�����յ�ģ�ͣ�Loop Subdivision���Ӻ��ʡ�
  
<img src='https://raw.githubusercontent.com/winka9587/MD_imgs/main/Norproject/2022-08-21-cBB8jC.png' width="100%" >

