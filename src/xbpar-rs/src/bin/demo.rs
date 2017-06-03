/***********************************************************************************************************************
 * Copyright (C) 2017 Robert Ou and contributors                                                                       *
 *                                                                                                                     *
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General   *
 * Public License as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) *
 * any later version.                                                                                                  *
 *                                                                                                                     *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied  *
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for     *
 * more details.                                                                                                       *
 *                                                                                                                     *
 * You should have received a copy of the GNU Lesser General Public License along with this program; if not, you may   *
 * find one here:                                                                                                      *
 * https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt                                                              *
 * or you may search the http://www.gnu.org website for the version 2.1 license, or you may write to the Free Software *
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA                                      *
 **********************************************************************************************************************/

extern crate xbpar_rs;
use xbpar_rs::*;

use std::collections::{HashMap, HashSet};

struct DeviceData {
    i: usize,
}

struct NetlistData {
}

struct TrivialPAREngine<'e, 'g: 'e> {
    base_engine: Option<&'e mut BasePAREngine<'e, 'g, DeviceData, NetlistData>>,
    label_map: HashMap<u32, &'static str>,
}

impl<'e, 'g: 'e> PAREngineImpl<'e, 'g, DeviceData, NetlistData> for TrivialPAREngine<'e, 'g> {
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

        //Pick one at random
        Some(candidates[(base_engine.random_number() % ncandidates) as usize])
    }

    fn find_suboptimal_placements(&'e mut self) -> Vec<&'g PARGraphNode<NetlistData, DeviceData>> {
        println!("find_suboptimal_placements");
        let base_engine = self.base_engine.as_mut().unwrap();

        let mut nodes = HashSet::new();

        // //Find all nodes that are on one end of an unroutable edge
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
        //For each label, mate each node in the netlist with the first legal mate in the device.
        //Simple and deterministic.
        let nmax_net = base_engine.get_graphs().n.get_max_label();
        for label in 0..(nmax_net + 1)
        {
            let nnet = base_engine.get_graphs().n.get_num_nodes_with_label(label);
            let nsites = base_engine.get_graphs().d.get_num_nodes_with_label(label);

            let mut nsite = 0;
            for net in 0..nnet
            {
                let netnode = base_engine.get_graphs().n.get_node_by_label_and_index(label, net).get_index();

                //If the netlist node is already constrained, don't auto-place it
                if base_engine.get_graphs().n.get_node_by_label_and_index(label, net).get_mate().is_some() {
                    continue;
                }

                //Try to find a legal site
                let mut found = false;
                while nsite < nsites
                {
                    let devnode = base_engine.get_graphs().d.get_node_by_label_and_index(label, nsite).get_index();
                    nsite += 1;

                    //If the site is used, we don't want to disturb what's already there
                    //because it was probably LOC'd
                    if base_engine.get_graphs().d.get_node_by_label_and_index(label, nsite).get_mate().is_some() {
                        continue;
                    }

                    //Site is unused, mate with it
                    base_engine.mate_nodes(netnode, devnode);
                    found = true;
                    break;
                }

                //This can happen in rare cases
                //(for example, we constrained all of the 8-bit counters to COUNT14 sites and now have a COUNT14).
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

fn main() {
    // Create a device graph with four nodes, two of type A and two of type B
    // Create a netlist graph with two nodes, one of type A and one of type B

    // The graphs
    let mut graphs = PARGraphPair::new_pair();

    // Labels
    let typea_label_d = graphs.borrow_mut_d().allocate_label();
    let typea_label_n = graphs.borrow_mut_n().allocate_label();
    assert_eq!(typea_label_d, typea_label_n);
    let typeb_label_d = graphs.borrow_mut_d().allocate_label();
    let typeb_label_n = graphs.borrow_mut_n().allocate_label();
    assert_eq!(typeb_label_d, typeb_label_n);
    let mut label_map = HashMap::new();
    label_map.insert(typea_label_d, "Type A node");
    label_map.insert(typeb_label_d, "Type B node");

    // Device graph
    let d_type_a_1 = graphs.borrow_mut_d().add_new_node(typea_label_d, DeviceData{i: 0});
    let d_type_a_2 = graphs.borrow_mut_d().add_new_node(typea_label_d, DeviceData{i: 1});
    let d_type_b_1 = graphs.borrow_mut_d().add_new_node(typeb_label_d, DeviceData{i: 2});
    let d_type_b_2 = graphs.borrow_mut_d().add_new_node(typeb_label_d, DeviceData{i: 3});

    graphs.borrow_mut_d().add_edge(d_type_a_1, "A to B", d_type_b_1, "B to A");
    graphs.borrow_mut_d().add_edge(d_type_a_1, "A to B", d_type_b_2, "B to A");
    graphs.borrow_mut_d().add_edge(d_type_a_2, "A to B", d_type_b_1, "B to A");
    graphs.borrow_mut_d().add_edge(d_type_a_2, "A to B", d_type_b_2, "B to A");

    // Netlist graph
    let n_type_a_1 = graphs.borrow_mut_n().add_new_node(typea_label_d, NetlistData{});
    let n_type_b_1 = graphs.borrow_mut_n().add_new_node(typeb_label_d, NetlistData{});

    graphs.borrow_mut_n().add_edge(n_type_a_1, "A to B", n_type_b_1, "B to A");

    // Do the thing!
    {
        let engine_impl = TrivialPAREngine {
            base_engine: None,
            label_map: label_map,
        };
        let mut engine_obj = PAREngine::new(engine_impl, &mut graphs);
        if !engine_obj.place_and_route(0) {
            panic!("PAR failed!");
        }
    }

    // Print out
    println!("Netlist A mate is: {}",
        graphs.borrow().n.get_node_by_index(n_type_a_1).get_mate().unwrap().get_associated_data().i);
    println!("Netlist B mate is: {}",
        graphs.borrow().n.get_node_by_index(n_type_b_1).get_mate().unwrap().get_associated_data().i);
}
