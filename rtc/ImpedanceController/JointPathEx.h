#ifndef __JOINT_PATH_EX_H__
#define __JOINT_PATH_EX_H__
#include <hrpModel/Body.h>
#include <hrpModel/Link.h>
#include <hrpModel/JointPath.h>

// hrplib/hrpUtil/MatrixSolvers.h
namespace hrp {
    int calcSRInverse(const dmatrix& _a, dmatrix &_a_sr, double _sr_ratio = 1.0, dmatrix _w = dmatrix::Identity(0,0));
};

// hrplib/hrpModel/JointPath.h
namespace hrp {
    class JointPathEx : public JointPath {
  public:
    JointPathEx(BodyPtr& robot, Link* base, Link* end);
    bool calcJacobianInverseNullspace(dmatrix &J, dmatrix &Jinv, dmatrix &Jnull);
    bool calcInverseKinematics2Loop(const Vector3& dp, const Vector3& omega, dvector &dq);
    bool calcInverseKinematics2(const Vector3& end_p, const Matrix33& end_R);
    double getSRGain() { return sr_gain; }
    bool setSRGain(double g) { sr_gain = g; }
    double getManipulabilityLimit() { return manipulability_limit; }
    bool setManipulabilityLimit(double l) { manipulability_limit = l; }
  protected:
        std::vector<Link*> joints;
        std::vector<double> avoid_weight_gain;
	double sr_gain, manipulability_limit;
    };

    typedef boost::shared_ptr<JointPathEx> JointPathExPtr;

};

#include <iomanip>

#endif //__JOINT_PATH_EX_H__
