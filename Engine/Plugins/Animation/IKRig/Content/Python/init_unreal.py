"""
This script will launch on Editor startup if Control Rig is enabled. The executions below are functional
examples on how users can use Python to extend the Control Rig Editor.
"""

if hasattr(unreal, 'RigVMUserWorkflowProvider'):
    import ControlRigWorkflows.workflow_fbik_import_ik_rig
    ControlRigWorkflows.workflow_fbik_import_ik_rig.provider.register()