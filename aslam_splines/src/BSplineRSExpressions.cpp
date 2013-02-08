#include <aslam/splines/BSplineRSExpressions.hpp>
#include <aslam/splines/BSplineRSPoseDesignVariable.hpp>
#include <sm/kinematics/transformations.hpp>

#define LINEDELAY_UNIT 1 //e-11

namespace aslam {
  namespace splines {
    
      /////////////////////////
      
      // linedelay in UNIT
      RSLineDelayTransformationExpressionNode::RSLineDelayTransformationExpressionNode(BSplinePoseDesignVariable * bspline, aslam::backend::DesignVariableVector<1>* lineDelay, double integrationStartTime, double line)
      {
          _spline = bspline;
          _lineDelay = lineDelay;
          _line = line;
          _integrationStartTime = integrationStartTime;
      }
      
      RSLineDelayTransformationExpressionNode::~RSLineDelayTransformationExpressionNode()
      {
          
      }
      
      
      Eigen::Matrix4d RSLineDelayTransformationExpressionNode::toTransformationMatrixImplementation()
      {
          
          double observationTime = _lineDelay->value()[0]* LINEDELAY_UNIT * _line + _integrationStartTime;
          return _spline->spline().transformation(observationTime);
          
      }
      
      void RSLineDelayTransformationExpressionNode::evaluateJacobiansImplementation(aslam::backend::JacobianContainer & outJacobians) const
      {
          
          double observationTime = _lineDelay->value()[0] * LINEDELAY_UNIT * _line + _integrationStartTime;
          Eigen::VectorXi dvidxs = _spline->spline().localVvCoefficientVectorIndices(observationTime);
          
          Eigen::MatrixXd J;
          _spline->spline().transformationAndJacobian(observationTime, &J);
          
          for(int i = 0; i < dvidxs.size(); ++i)
          {
              outJacobians.add(_spline->designVariable(dvidxs[i]), J.block<6,6>(0,i*6) );
          }        
          
          // evaluate time derivative of the curves
          Eigen::VectorXd Phi_dot_c = _spline->spline().evalD(observationTime,1); // phi_dot * c (t_0)
          // spline parameters:
          Eigen::VectorXd r = _spline->spline().evalD(observationTime,0);
          // rot kinematics:
          sm::kinematics::RotationalKinematics::Ptr R = _spline->spline().rotation();
          // S-Matrix
          Eigen::Matrix3d S = R->parametersToSMatrix(r.tail<3>()); 
          
          // Add the jacobians wrt line delay: \mbf S * \mbsdot \Phi(t) * c * p_{i,v}
          outJacobians.add(_lineDelay, S * Phi_dot_c * _line * LINEDELAY_UNIT);
          
      }
      
      void RSLineDelayTransformationExpressionNode::evaluateJacobiansImplementation(aslam::backend::JacobianContainer & outJacobians, const Eigen::MatrixXd & applyChainRule) const
      {
          
          Eigen::MatrixXd JT;
          Eigen::MatrixXd J;
          
          
          double observationTime = _lineDelay->value()[0] * LINEDELAY_UNIT * _line + _integrationStartTime;
          
          Eigen::VectorXi dvidxs = _spline->spline().localVvCoefficientVectorIndices(observationTime);
          // _spline->spline().transformationAndJacobian(observationTime, &J);
          Eigen::MatrixXd JS;
          Eigen::VectorXd p;
          
          p = _spline->spline().evalDAndJacobian(observationTime,0,&JS, NULL);
           
          
          Eigen::Matrix4d T = _spline->spline().curveValueToTransformationAndJacobian( p, &JT );    
          J = JT * JS;  
          
          for(int i = 0; i < dvidxs.size(); ++i)
          {              
              outJacobians.add(_spline->designVariable(dvidxs[i]), applyChainRule * J.block<6,6>(0,i*6) );
          }
          // evaluate time derivative of the curves
          Eigen::VectorXd Phi_dot_c = _spline->spline().evalD(observationTime,1); // phi_dot * c (t_0)
          
          
          // Add the jacobians wrt line delay: \mbf S_T * \mbsdot \Phi_dot(t) * c * p_{i,v}
          outJacobians.add(_lineDelay, applyChainRule * JT * Phi_dot_c * _line * LINEDELAY_UNIT); //
          
      }
      
      void RSLineDelayTransformationExpressionNode::getDesignVariablesImplementation(aslam::backend::JacobianContainer::set_t & designVariables) const
      {
          double observationTime = _lineDelay->value()[0] * LINEDELAY_UNIT * _line + _integrationStartTime;
          Eigen::VectorXi dvidxs = _spline->spline().localVvCoefficientVectorIndices(observationTime);
          for(int i = 0; i < dvidxs.size(); ++i)
          {
              designVariables.insert( _spline->designVariable(dvidxs[i]) );
          }
          designVariables.insert(_lineDelay);
      }



  } // namespace splines
} // namespace aslam
