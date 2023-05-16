
import numpy as np
import random
import typing
from copy import deepcopy
'''
生成多播需求+处理输出结果，最后生成需求文件multicast_schedule.csv
'''

N_SAT = 1156 # 卫星数量 kuiper(1156)
N_GS = 100 # 地面站数量
TIME_INTERVAL_MS = 1000 # 仿真时间间隔（ms）
SIM_DURATION_S = 4 # 仿真总时间（s）
RATE_DEFAULE = 10 # 默认速率 Mbps

class Mreq:
    def __init__(self, src, dsts:typing.List[int], rate, t_start, t_dura) -> None:
        self.id = -1 #do not set init id，因为预处理时id没用且维护很麻烦
        self.src = src 
        self.dsts = dsts
        self.rate = rate # Mbit/s
        self.time_start = t_start # ns
        self.time_dura = t_dura # ns
        self.metadata = {}
    
    def addmeta(self, key, value): 
        '''add a k-v pair in metadata'''
        self.metadata[key] = value
    
    def set_id(self, x):
        self.id = x

    def set_timestart(self, x):
        self.time_start = x
    
    def set_timedura(self, x):
        self.time_dura = x
    
    def check_dura(self, timeleft_ns, timeright_ns):
        '''时间上是否有交集'''
        time_end = self.time_start + self.time_dura
        return (self.time_start < timeright_ns) and (time_end > timeleft_ns)
    
    def show(self):
        print("mreq: id={}  src={}  dsts={}  rate={}  time_start={}  time_dura={}  metadata={}".format(self.id, self.src, self.dsts, self.rate, self.time_start, self.time_dura, self.metadata))


def gen_rawreqs_random(num = 10) -> typing.List[Mreq]: #原始需求，不处理时间片和src分割
    reqs = []
    # node_id_sat = (0, N_SAT)
    node_id_gs = (N_SAT, N_SAT+N_GS)
    for i in range(num): #生成num个req
        #随机选择src（随机整数）
        src = np.random.randint(*node_id_gs)
        #随机选择dst个数（泊松分布）
        dst_num = 0
        while dst_num == 0: #避免0个dst
            dst_num = np.random.poisson(int(N_GS//20))
        #随机选择dst（随机均匀采样）
        cand_dsts = list(range(*node_id_gs))
        cand_dsts.remove(src) #src不能作为dst
        dsts = np.random.choice(cand_dsts, dst_num, replace=False).tolist()
        #速率统一为100
        rate = RATE_DEFAULE
        #随机选择持续时间（指数分布）
        dura = np.random.exponential(SIM_DURATION_S*0.2e6) #转化为ms
        dura = int(dura*1000) #ns
        #随机选择起始时间(随机选择中位数-持续时间/2)
        mid_dura = np.random.randint(0, SIM_DURATION_S*1e9) #ns
        time_start = max(mid_dura-dura//2, 0) #至少0开始
        time_end = min(mid_dura+dura//2, SIM_DURATION_S*1e9) #最多SIM_DURATION_S结束
        dura = time_end-time_start
        #更新reqs
        reqs.append(Mreq(src=src, dsts=dsts, rate=rate, t_start=time_start, t_dura=dura))

    return reqs


def req_split_bytime(): #对需求按时间片分割
    '''
    对需求按时间片分割，如果成员没有动态变化，这一步可以不做？
    '''
    pass

def load_fstate(dir, t, fstate_pre=None): #读取转发状态
    '''
    dir: 转发状态对应的目录
    t:当前时间，单位ms
    要求对应文件一定存在：SIM_DURA不能超过fstate计算的长度
    '''
    n_node = N_GS + N_SAT
    file_name = dir + "/fstate_{}.txt".format(int(t*1e6)) #转化为ns
    #初始化fstate，如果是t=0那么需要初始化一个
    fstate = fstate_pre
    if t == 0:
        print("load_fstate: t == 0, init fstate...")
        fstate = [{dst:{"nexthop":-1, "oif":-1, "iif":-1} for dst in range(N_SAT, n_node)} for src in range(n_node)] #第一层是src，包括所有节点；第二层只有gs
    with open(file_name, 'r') as f:
        for line in f:
            sline = line.split(',')
            src = int(sline[0])
            dst = int(sline[1])
            nxt = int(sline[2])
            oif = int(sline[3])
            iif = int(sline[4])
            fstate[src][dst]['nexthop'] = nxt
            fstate[src][dst]['oif'] = oif
            fstate[src][dst]['iif'] = iif
    return fstate

def req_split_atsrc(reqs:typing.List[Mreq], fstate_dir): #对需求按src的端口分割
    def check_src_routing_changed(req:Mreq, fstate_pre, fstate_cur):
        '''
        检查前后fstate变化，有变化需要切time slot，否则不需要切
        即t和t+1的src处拆分情况不同，则需要按time切成两个req
        '''
        if fstate_pre is None: #初始
            return False #不需要切，所以false
        src = req.src
        #记录等价类
        dst_group_pre = {}
        dst_group_cur = {}
        for dst in req.dsts:
            cnxt = fstate_cur[src][dst]['nexthop']
            coif = fstate_cur[src][dst]['oif']
            dgc = dst_group_cur.setdefault((cnxt, coif), set())
            dgc.add(dst)

            pnxt = fstate_pre[src][dst]['nexthop']
            poif = fstate_pre[src][dst]['oif']
            dgp = dst_group_pre.setdefault((pnxt, poif), set())
            dgp.add(dst)
        #判断差异
        if dst_group_cur.keys() != dst_group_pre.keys():
            return True
        for key in dst_group_pre:
            if dst_group_pre[key] != dst_group_cur[key]:
                return True
        return False
    
    def split_bydst_single_slot(req:Mreq, fstate):
        src = req.src
        idx = req.id
        dst_group = {} # 按 group <nexthop, oif> 分组
        for dst in req.dsts:
            nxthop = fstate[src][dst]['nexthop']
            oif = fstate[src][dst]['oif']
            dg = dst_group.setdefault((nxthop, oif), set())
            dg.add(dst)
        sp_reqs = []
        for key in dst_group:
            cur_req = Mreq(src=src, dsts=list(dst_group[key]), rate=req.rate, t_start=req.time_start, t_dura=req.time_dura)
            cur_req.addmeta("origin_id", idx)
            sp_reqs.append(cur_req)
        return sp_reqs

    def req_fork_time(req:Mreq, time_ns):
        '''
        将req从time处切为前后两个，return新的req，修改原req的time_start和time_dura
                start       dura
        原req： time_ns+1   t_end-time_ns
        newreq：t_start     time_ns-t_start
        '''
        assert(req.time_start < time_ns <= req.time_start + req.time_dura)
        reqnew = Mreq(src=req.src, dsts=req.dsts.copy(), rate=req.rate, t_start=req.time_start, t_dura=time_ns-req.time_start)
        req.set_timedura(req.time_start+req.time_dura-time_ns) #这里set的前后顺序不能变
        req.set_timestart(time_ns) # must+1
        # req.set_timestart(time_ns+1) # must+1
        return reqnew

    #读取fstate
    total_dura = SIM_DURATION_S * 1000 #ms
    time_interval = TIME_INTERVAL_MS
    assert(total_dura % time_interval == 0) #必须整除
    req_final = []

    req_forked = []
    fstate_pre = load_fstate(dir=fstate_dir, t=0, fstate_pre=None)
    for tright_ms in range(time_interval, total_dura+1, time_interval): #时间轴右边界
        tright_ns = tright_ms*1e6
        time_interval_ns = time_interval * 1e6
        fstate = load_fstate(dir=fstate_dir, t=tright_ms, fstate_pre=deepcopy(fstate_pre)) #下一时刻路由
        for req in reqs:
            if tright_ns - time_interval_ns <= req.time_start + req.time_dura < tright_ns:
                req_final = req_final + split_bydst_single_slot(req=req, fstate=fstate_pre)
            if not req.time_start < tright_ns <= req.time_start+req.time_dura: #如果tright不在req时间范围内，不需要管
                continue
            if req.time_start < tright_ns <= req.time_start+req.time_dura and \
                check_src_routing_changed(req, fstate_pre, fstate): #如果前后变化了,判断变化的前提是tright两侧都有需求
                #拆分成前后两段
                reqnew = req_fork_time(req, tright_ns) #从tright_ns开始会采用新的route
                req_forked.append(reqnew)
                #fork出来的可以安全split
                req_final = req_final + split_bydst_single_slot(req=reqnew, fstate=fstate_pre)
            # else: #不变的通常先不切，但是如果已经到末尾了，就切
            #     pass 
        fstate_pre = fstate # prepare for update
    
    print("req_forked:")
    for r in req_forked:
        r.show()
    print("\n\nreq_origin")
    for r in reqs:
        r.show()

    return req_final

def reqs_to_csv(reqs:typing.List[Mreq], tar_filename):
    #按照starttime重排序
    reqs.sort(key=lambda x:x.time_start)
    with open(tar_filename, 'w') as f:
        i = 0
        for req in reqs:
            dsts_str = " ".join([str(x) for x in req.dsts])
            f.write("{id},{src},{ndst},{strdsts},{rate},{timestart:.0f},{timedura:.0f},,\n".format(id=i, src=req.src, ndst=len(req.dsts), strdsts=dsts_str, rate=req.rate, timestart=req.time_start, timedura=req.time_dura))
            i = i+1

def main():
    # print('****** gen multicast reqs -random ******')
    # random gen
    # np.random.seed(77)
    reqs = gen_rawreqs_random(num = 100)
    
    #static
    # 0,1243,2,1161 1241,100,1018444738,814043676,,
    # req = Mreq(src=1243, dsts=[1161,1241], rate=100, t_start=1018444738, t_dura=814043676)
    # reqs = [req]

    for req in reqs:
        req.show()

    print("****** multicast reqs pre proc ******")
    fstate_dir = "../sat_state/kuiper_630_isls_plus_grid_ground_stations_top_100_algorithm_free_one_only_over_isls/dynamic_state_{}ms_for_200s".format(TIME_INTERVAL_MS)
    reqfinal = req_split_atsrc(reqs, fstate_dir)

    print("\n\n\nreqs after proc")
    for r in reqfinal:
        r.show()

    print("****** write file to csv ******")
    tarfile = './multicast_test/multicast_schedule.csv'
    reqs_to_csv(reqs=reqfinal, tar_filename=tarfile)

if __name__ == '__main__':
    main()