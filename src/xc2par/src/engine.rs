/*
Copyright (c) 2016-2017, Robert Ou <rqou@robertou.com> and contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

use std::collections::{HashMap, HashSet};

extern crate xbpar_rs;
use self::xbpar_rs::*;

use *;
use objpool::*;

type DeviceData = ObjPoolIndex<DeviceGraphNode>;
type NetlistData = ObjPoolIndex<NetlistGraphNode>;

pub struct XC2PAREngine<'e, 'g: 'e> {
    base_engine: Option<&'e mut BasePAREngine<'e, 'g, DeviceData, NetlistData>>,
    label_map: HashMap<u32, &'static str>,
}

impl<'e, 'g: 'e> XC2PAREngine<'e, 'g> {
    pub fn new(lmap: HashMap<u32, &'static str>) -> Self {
        XC2PAREngine {
            base_engine: None,
            label_map: lmap,
        }
    }
}

impl<'e, 'g: 'e> PAREngineImpl<'e, 'g, DeviceData, NetlistData> for XC2PAREngine<'e, 'g> {
    fn set_base_engine(&'e mut self, base_engine: &'e mut BasePAREngine<'e, 'g, DeviceData, NetlistData>) {
        self.base_engine = Some(base_engine);
    }

    fn can_move_node(&'e mut self, node: &'g PARGraphNode<NetlistData, DeviceData>,
        old_mate: &'g PARGraphNode<DeviceData, NetlistData>,
        new_mate: &'g PARGraphNode<DeviceData, NetlistData>) -> bool {

        println!("can_move_node");
        let base_engine = self.base_engine.as_mut().unwrap();
        base_engine.can_move_node(node, old_mate, new_mate)
    }

    fn get_new_placement_for_node(&'e mut self, pivot: &'g PARGraphNode<NetlistData, DeviceData>)
        -> Option<&'g PARGraphNode<DeviceData, NetlistData>> {

        println!("get_new_placement_for_node");
        let base_engine = self.base_engine.as_mut().unwrap();
        let m_device = base_engine.get_graphs().d;

        let label = pivot.get_label();

        let mut candidates = Vec::new();
        for i in 0..m_device.get_num_nodes_with_label(label) {
            candidates.push(m_device.get_node_by_label_and_index(label, i));
        }

        let ncandidates = candidates.len() as u32;
        if ncandidates == 0 {
            return None;
        }

        // Pick one at random
        Some(candidates[(base_engine.random_number() % ncandidates) as usize])
    }

    fn find_suboptimal_placements(&'e mut self) -> Vec<&'g PARGraphNode<NetlistData, DeviceData>> {
        println!("find_suboptimal_placements");
        let base_engine = self.base_engine.as_mut().unwrap();

        let mut nodes = HashSet::new();

        // Find all nodes that are on one end of an unroutable edge
        let (_, unroutes) = base_engine.compute_unroutable_cost();
        for edge in unroutes
        {
            nodes.insert(edge.sourcenode());
            nodes.insert(edge.destnode());
        }

        nodes.into_iter().collect()

    }

    fn compute_and_print_score(&'e mut self, iteration: u32) -> (u32, Vec<&'g PARGraphEdge<NetlistData, DeviceData>>) {
        println!("compute_and_print_score");
        let base_engine = self.base_engine.as_mut().unwrap();
        base_engine.compute_and_print_score(iteration)
    }

    fn print_unroutes(&'e mut self, unroutes: &[&'g PARGraphEdge<NetlistData, DeviceData>]) {
        println!("print_unroutes");
        let base_engine = self.base_engine.as_mut().unwrap();
        base_engine.print_unroutes(unroutes)
    }

    fn compute_congestion_cost(&mut self) -> u32 {
        println!("compute_congestion_cost");
        let base_engine = self.base_engine.as_mut().unwrap();
        base_engine.compute_congestion_cost()
    }

    fn compute_timing_cost(&mut self) -> u32 {
        println!("compute_timing_cost");
        let base_engine = self.base_engine.as_mut().unwrap();
        base_engine.compute_timing_cost()
    }

    fn compute_unroutable_cost(&'e mut self) -> (u32, Vec<&'g PARGraphEdge<NetlistData, DeviceData>>) {
        println!("compute_unroutable_cost");
        let base_engine = self.base_engine.as_mut().unwrap();
        base_engine.compute_unroutable_cost()
    }

    fn sanity_check(&mut self) -> bool {
        println!("sanity_check");
        let base_engine = self.base_engine.as_mut().unwrap();
        base_engine.sanity_check()
    }

    fn initial_placement(&mut self) -> bool {
        println!("initial_placement");
        let base_engine = self.base_engine.as_mut().unwrap();
        base_engine.initial_placement()
    }

    fn initial_placement_core(&mut self) -> bool {
        println!("initial_placement_core");
        let base_engine = self.base_engine.as_mut().unwrap();
        // For each label, mate each node in the netlist with the first legal mate in the device.
        // Simple and deterministic.
        let nmax_net = base_engine.get_graphs().n.get_max_label();
        for label in 0..(nmax_net + 1)
        {
            let nnet = base_engine.get_graphs().n.get_num_nodes_with_label(label);
            let nsites = base_engine.get_graphs().d.get_num_nodes_with_label(label);

            let mut nsite = 0;
            for net in 0..nnet
            {
                let netnode = base_engine.get_graphs().n.get_node_by_label_and_index(label, net).get_index();

                // If the netlist node is already constrained, don't auto-place it
                if base_engine.get_graphs().n.get_node_by_label_and_index(label, net).get_mate().is_some() {
                    continue;
                }

                // Try to find a legal site
                let mut found = false;
                while nsite < nsites
                {
                    let devnode = base_engine.get_graphs().d.get_node_by_label_and_index(label, nsite).get_index();
                    nsite += 1;

                    // If the site is used, we don't want to disturb what's already there
                    // because it was probably LOC'd
                    if base_engine.get_graphs().d.get_node_by_label_and_index(label, nsite).get_mate().is_some() {
                        continue;
                    }

                    // Site is unused, mate with it
                    base_engine.mate_nodes(netnode, devnode);
                    found = true;
                    break;
                }

                // This can happen in rare cases
                // (for example, we constrained all of the 8-bit counters to COUNT14 sites and now have a COUNT14).
                if !found
                {
                    return false;
                }
            }
        }

        true
    }

    fn optimize_placement(&'e mut self, badnodes: &[&'g PARGraphNode<NetlistData, DeviceData>]) -> bool {
        println!("optimize_placement");
        let base_engine = self.base_engine.as_mut().unwrap();
        base_engine.optimize_placement(badnodes)
    }

    fn get_label_name(&mut self, label: u32) -> &str {
        self.label_map[&label]
    }

    fn compute_node_unroutable_cost(&'e mut self,
        pivot: &'g PARGraphNode<NetlistData, DeviceData>,
        candidate: &'g PARGraphNode<DeviceData, NetlistData>) -> u32 {

        println!("compute_node_unroutable_cost");
        let base_engine = self.base_engine.as_mut().unwrap();
        base_engine.compute_node_unroutable_cost(pivot, candidate)
    }
}
