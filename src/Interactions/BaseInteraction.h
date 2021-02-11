/*
 * BaseInteraction is a class that is meant "contain" all the interaction
 * types
 */

#ifndef BASE_INTERACTION_H
#define BASE_INTERACTION_H

#include "../defs.h"
#include "../Particles/BaseParticle.h"
#include "../Boxes/BaseBox.h"
#include "../Lists/BaseList.h"
#include "../Utilities/Utils.h"
#include "../Utilities/oxDNAException.h"
#include "Mesh.h"

#include "../Lists/Cells.h"

#include <map>
#include <cfloat>
#include <fstream>
#include <set>
#include <vector>
#include <array>
#include <functional>

#define ADD_INTERACTION_TO_MAP(index, member) {_interaction_map[index] = [this](BaseParticle *p, BaseParticle *q, bool compute_r, bool compute_forces) { return member(p, q, compute_r, compute_forces); };}

using StressTensor = std::array<number, 6>;

/**
 * @brief Abstract class defining the interaction interface.
 *
 * We implement the Curiously Recurring Template Pattern (see
 * http://en.wikipedia.org/wiki/Curiously_recurring_template_pattern) and therefore
 * we have to keep this class and BaseInteraction separated.
 */
class IBaseInteraction {
protected:
	BaseBox *_box;

	/// This is useful for "hard" potentials
	bool _is_infinite;

	/// This are needed to create initial configurations, and they are used by the generator functions
	number _energy_threshold, _temperature;
	/// If true, the generation of the initial configuration will try to take into account the fact that particles that are bonded neighbours should be close to each other
	bool _generate_consider_bonded_interactions;
	/// This controls the maximum at which bonded neighbours should be randomly placed to speed-up generation. Used by generator functions.
	number _generate_bonded_cutoff;

	number _rcut, _sqr_rcut;

	char _topology_filename[256];

	LR_vector _computed_r;

	StressTensor _stress_tensor;

	virtual void _update_stress_tensor(LR_vector r_p, LR_vector group_force);

public:
	IBaseInteraction();
	virtual ~IBaseInteraction();

	virtual void set_box(BaseBox *box) {
		_box = box;
	}

	virtual void get_settings(input_file &inp);

	/**
	 * Initialization of class constants.
	 */
	virtual void init() = 0;

	/**
	 * @brief Handles particle allocation. Child classes must implement it.
	 *
	 * @param particles
	 * @param N
	 */
	virtual void allocate_particles(std::vector<BaseParticle *> &particles) = 0;

	/**
	 * @brief Returns the maximum cut-off associated to the interaction
	 *
	 * @return Interaction cutoff
	 */
	virtual number get_rcut() {
		return _rcut;
	}

	/**
	 * @brief Check whether the initial configuration makes sense.
	 *
	 * Since this operation is interaction-dependent, each interaction must provide its own implementation.
	 *
	 * @param particles
	 * @param N
	 */
	virtual void check_input_sanity(std::vector<BaseParticle *> &particles) = 0;

	/**
	 * @brief Signals the interaction that an energy (or force) computation is about to begin.
	 *
	 * By default this method resets the stress tensor data structures and nothing else, but interactions inheriting from this interface may
	 * need to initialise or reset other data structures before computing the energy or the force acting on all particles.
	 *
	 */
	virtual void begin_energy_computation();

	/**
	 * @brief Returns true if the interaction computes the stress tensor internally, false otherwise.
	 *
	 * This method will return false if not overridden.
	 *
	 * @return
	 */
	virtual bool has_custom_stress_tensor() const {
		return false;
	}

	void reset_stress_tensor();

	StressTensor stress_tensor() const {
		return _stress_tensor;
	}

	/**
	 * @brief Computes the total interaction between particles p and q.
	 *
	 * If r is not given or NULL, it is computed from scratch. It can optionally update forces and torques exerted on p and q.
	 * @param p
	 * @param q
	 * @param r
	 * @param update_forces
	 * @return pair-interaction energy
	 */
	virtual number pair_interaction(BaseParticle *p, BaseParticle *q, bool compute_r = true, bool update_forces = false) = 0;

	/**
	 * @brief Computes the bonded part interaction between particles p and q.
	 *
	 * See {\@link pair_interaction} for a description of the parameters.
	 */
	virtual number pair_interaction_bonded(BaseParticle *p, BaseParticle *q, bool compute_r = true, bool update_forces = false) = 0;

	/**
	 * @brief Computed the non bonded part of the interaction between particles p and q.
	 *
	 * See {\@link pair_interaction} for a description of the parameters.
	 */
	virtual number pair_interaction_nonbonded(BaseParticle *p, BaseParticle *q, bool compute_r = true, bool update_forces = false) = 0;

	/**
	 * @brief Computes the requested term of the interaction energy between p and q.
	 *
	 * @param name identifier of the interaction method
	 * @param p
	 * @param q
	 * @param r
	 * @param update_forces
	 * @return
	 */
	virtual number pair_interaction_term(int name, BaseParticle *p, BaseParticle *q, bool compute_r = true, bool update_forces = false) = 0;

	/**
	 * @brief Returns the total potential energy of the system, given a box size
	 *
	 * @param particles
	 * @param N
	 * @return
	 */
	virtual number get_system_energy(std::vector<BaseParticle *> &particles, BaseList *lists);

	/**
	 * @brief Like get_system_energy_term, just that the box size can be specified in this case
	 *
	 * @param name
	 * @param particles
	 * @param N
	 * @return the required energy contribution
	 */
	virtual number get_system_energy_term(int name, std::vector<BaseParticle *> &particles, BaseList *lists);

	/**
	 * @brief Read the topology of the interactions. The defaut
	 * method is empty.
	 *
	 * This function is set up so that the topology of the interactions
	 * are read in the code. The different interactions might require
	 * different formats, so it is implemented here. The default
	 *
	 * @param N_strands Pointer to an int which will be filled with the
	 * number of "strands" (unbreakable clusters of connected particles)
	 * @param particles array of particles.
	 */
	virtual void read_topology(int *N_strands, std::vector<BaseParticle *> &particles);

	/**
	 * @brief Returns the number of particles, as written in the topology.
	 *
	 * @return number of particles
	 */
	virtual int get_N_from_topology();

	/**
	 * @brief Computed the system energy, split into different terms, which depend on the specific interaction considered.
	 *
	 * @return map of the energy contributions
	 */
	virtual std::map<int, number> get_system_energy_split(std::vector<BaseParticle *> &particles, BaseList *lists) = 0;

	/**
	 * @brief Returns the state of the interaction
	 */
	bool get_is_infinite() {
		return _is_infinite;
	}

	/**
	 * @brief Sets _is_infinite
	 *
	 * @param arg bool
	 */
	void set_is_infinite(bool arg) {
		_is_infinite = arg;
	}

	/**
	 * @brief Sets the distance between particles to be used by the pair_interaction_* methods.
	 *
	 * @param r the new particle distance
	 */
	void set_computed_r(LR_vector &r) {
		_computed_r =r;
	}

	/**
	 * @brief Check whether the two particles overlaps. Used only in the generation of configurations. Can be overloaded.
	 *
	 * The default implementation does not allow two particles to have an
	 * interaction strength greater than 100. More subtle implementations
	 * may be needed in special cases.
	 *
	 * @param p
	 * @param q
	 * @return bool whether the two particles overlap
	 */
	virtual bool generate_random_configuration_overlap(BaseParticle *p, BaseParticle *q);

	/**
	 * @brief Generate an initial configuration. Can be overloaded.
	 *
	 * The default function creates a random configuration in the most simple way possible:
	 * puts particles in one at a time, and using {\@link generate_random_configuration_overlap}
	 * to test if two particles overlap.
	 *
	 * @param particles
	 * @param N
	 */
	virtual void generate_random_configuration(std::vector<BaseParticle *> &particles);
};

using InteractionPtr = std::shared_ptr<IBaseInteraction>;

/**
 * @brief Base class for managing particle-particle interactions. It is an abstract class.
 *
 * This is the abstract class from which the other interaction classes inherit.
 * The children classes must implement all the public pair_interaction_*
 * methods.  We make use of the Curiously Recurring Template Pattern (see
 * http://en.wikipedia.org/wiki/Curiously_recurring_template_pattern) to make
 * it possible to call a method of any derived class from within this class
 * without knowing any detail of that derived class. Hence, we use the {\@link
 * _int_map} map to store function pointers of type child::*.
 * This class lacks a .cpp because we do not want to have template
 * instantiation of the children classes from within here.
 */
template<typename child>
class BaseInteraction: public IBaseInteraction {
private:

protected:

	using energy_function = std::function<number(BaseParticle *, BaseParticle *, bool, bool)>;
	using interaction_map = std::map<int, energy_function>;
	interaction_map _interaction_map;

	/**
	 * @brief Calls the right interaction-computing method, chosen according to its name.
	 *
	 * @param that instance of the child class that actually calls the right method
	 * @param name identifier of the interaction method
	 * @param p
	 * @param q
	 * @param r
	 * @param update_forces
	 * @return pair-interaction energy
	 */
	virtual number _pair_interaction_term_wrapper(child *that, int name, BaseParticle *p, BaseParticle *q, bool compute_r, bool update_forces);

	/**
	 * @brief Build a mesh by using a function and its derivative.
	 *
	 * Only derived classes can call this function.
	 * @param that pointer to the class owning f and der
	 * @param f function
	 * @param der derivative of f
	 * @param pars pointer to a structure which contains function parameters
	 * @param npoints size of the mesh
	 * @param xlow the mesh is defined on a finite interval. This is the lower end
	 * @param xupp Upper end of the mesh interval
	 * @param m mesh to be built
	 */
	virtual void _build_mesh(child *that, number (child::*f)(number, void*), number (child::*der)(number, void*), void *pars, int npoints, number xlow, number xupp, Mesh &m);
	inline number _query_mesh(number x, Mesh &m);
	inline number _query_meshD(number x, Mesh &m);

public:
	/**
	 * @brief Basic constructor. By default, it does not need anything.
	 */
	BaseInteraction();
	virtual ~BaseInteraction();

	/**
	 * @brief Computes all the contributions to the total potential energy and returns them in a map.
	 *
	 * @param particles
	 * @param N
	 * @return a map storing all the potential energy contributions.
	 */
	virtual std::map<int, number> get_system_energy_split(std::vector<BaseParticle *> &particles, BaseList *lists);
};

template<typename child>
BaseInteraction<child>::BaseInteraction() :
				IBaseInteraction() {
	_rcut = 2.5;
	_sqr_rcut = SQR(_rcut);
}

template<typename child>
BaseInteraction<child>::~BaseInteraction() {

}

template<typename child>
number BaseInteraction<child>::_pair_interaction_term_wrapper(child *that, int name, BaseParticle *p, BaseParticle *q, bool compute_r, bool update_forces) {
	if(compute_r) {
		_computed_r = _box->min_image(p->pos, q->pos);
	}

	// _int_map.find(name) returns an iterator which points to the
	// requested function pointer. We dereference it, apply it to the
	// that, which is a pointer to an instance of a class which inherits
	// from BaseInteraction. We then pass in the required parameters and
	// return the interaction energy.
	typename interaction_map::iterator interaction = _interaction_map.find(name);
	if(interaction == _interaction_map.end()) {
		throw oxDNAException("%s, line %d: Interaction term '%d' not found", __FILE__, __LINE__, name);
	}

	return interaction->second(p, q, false, update_forces);
}

template<typename child>
inline number BaseInteraction<child>::_query_meshD(number x, Mesh &m) {
	if(x < m.xlow) return m.B[0];
	if(x >= m.xupp) x = m.xupp - FLT_EPSILON;
	int i = (int) ((x - m.xlow) / m.delta);
	number dx = x - m.xlow - m.delta * i;
	return (m.B[i] + (2 * dx * m.C[i] + 3 * dx * dx * m.D[i]) * m.inv_sqr_delta);
}

template<typename child>
inline number BaseInteraction<child>::_query_mesh(number x, Mesh &m) {
	if(x <= m.xlow) return m.A[0];
	if(x >= m.xupp) x = m.xupp - FLT_EPSILON;
	int i = (int) ((x - m.xlow) / m.delta);
	number dx = x - m.xlow - m.delta * i;
	return (m.A[i] + dx * (m.B[i] + dx * (m.C[i] + dx * m.D[i]) * m.inv_sqr_delta));
}

template<typename child>
void BaseInteraction<child>::_build_mesh(child *that, number (child::*f)(number, void*), number (child::*der)(number, void*), void * args, int npoints, number xlow, number xupp, Mesh &m) {
	assert(xlow < xupp);
	int i;
	number x;

	m.init(npoints);

	number dx = (xupp - xlow) / (number) npoints;
	m.delta = dx;
	m.inv_sqr_delta = 1 / SQR(dx);
	m.xlow = xlow;
	m.xupp = xupp;

	number fx0, fx1, derx0, derx1;

	for(i = 0; i < npoints + 1; i++) {
		x = xlow + i * dx;

		fx0 = (that->*f)(x, args);
		fx1 = (that->*f)(x + dx, args);
		derx0 = (that->*der)(x, args);
		derx1 = (that->*der)(x + dx, args);

		m.A[i] = fx0;
		m.B[i] = derx0;
		m.D[i] = (2 * (fx0 - fx1) + (derx0 + derx1) * dx) / dx;
		m.C[i] = (fx1 - fx0 + (-derx0 - m.D[i]) * dx);
	}
}

template<typename child>
std::map<int, number> BaseInteraction<child>::get_system_energy_split(std::vector<BaseParticle *> &particles, BaseList *lists) {
	begin_energy_computation();

	std::map<int, number> energy_map;

	for(auto it = _interaction_map.begin(); it != _interaction_map.end(); it++) {
		int name = it->first;
		energy_map[name] = (number) 0.f;
	}

	for(auto p: particles) {
		std::vector<BaseParticle *> neighs = lists->get_all_neighbours(p);

		for(unsigned int n = 0; n < neighs.size(); n++) {
			BaseParticle *q = neighs[n];
			if(p->index > q->index) {
				for(auto it = _interaction_map.begin(); it != _interaction_map.end(); it++) {
					int name = it->first;
					energy_map[name] += pair_interaction_term(name, p, q);
				}
			}
		}
	}

	return energy_map;
}

#endif /* BASE_INTERACTION_H */
