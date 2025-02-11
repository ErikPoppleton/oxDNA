/*
 * TotalEnergy.cpp
 *
 *  Created on: Feb 12, 2013
 *      Author: rovigatti
 */

#include <sstream>

#include "TotalEnergy.h"

// Initialization of static data members

llint TotalEnergy::_saved_on = -1;

number TotalEnergy::_U = (number) 0.;

number TotalEnergy::_K = (number) 0.;

TotalEnergy::TotalEnergy() {

}

TotalEnergy::~TotalEnergy() {

}

void TotalEnergy::get_settings(input_file &my_inp, input_file &sim_inp) {
	BaseObservable::get_settings(my_inp, sim_inp);

	_pot_energy.get_settings(my_inp, sim_inp);
	_kin_energy.get_settings(my_inp, sim_inp);
}

void TotalEnergy::init() {
	_pot_energy.init();
	_kin_energy.init();
}

std::string TotalEnergy::get_output_string(llint curr_step) {
	number U = get_U(curr_step);
	number K = get_K(curr_step);

	return Utils::sformat("% 10.6lf % 10.6lf % 10.6lf", U, K, U + K);
}

number TotalEnergy::get_U(llint curr_step) {
	if(curr_step > _saved_on) TotalEnergy::_U = _pot_energy.get_potential_energy();

	return TotalEnergy::_U;
}

number TotalEnergy::get_K(llint curr_step) {
	if(curr_step > _saved_on) TotalEnergy::_K = _kin_energy.get_kinetic_energy();

	return TotalEnergy::_K;
}
