## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

def build(bld):
    obj = bld.create_ns3_program('ocean_net',
                                 ['wifi', 'internet','stdma','flow-monitor','applications','aodv','olsr','dsdv','dsr'])
    obj.source = ['ocean-net.cc','mypacket.cc']
