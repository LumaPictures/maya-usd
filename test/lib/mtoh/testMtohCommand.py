import sys
import unittest

import maya.cmds as cmds
import maya.mel as mel

import mtohUtils

class TestCommand(unittest.TestCase):
    _file = __file__

    @classmethod
    def setUpClass(cls):
        cmds.loadPlugin('mtoh', quiet=True)

    def test_invalidFlag(self):
        self.assertRaises(TypeError, cmds.mtoh, nonExistantFlag=1)

    def test_listRenderers(self):
        renderers = cmds.mtoh(listRenderers=1)
        self.assertEqual(renderers, cmds.mtoh(lr=1))
        self.assertIn(mtohUtils.HD_STORM, renderers)
        self.assertIn("HdEmbreeRendererPlugin", renderers)

    def test_listActiveRenderers(self):
        activeRenderers = cmds.mtoh(listActiveRenderers=1)
        self.assertEqual(activeRenderers, cmds.mtoh(lar=1))
        self.assertEqual(activeRenderers, [])

        activeEditor = cmds.playblast(ae=1)
        cmds.modelEditor(
            activeEditor, e=1,
            rendererOverrideName=mtohUtils.HD_STORM_OVERRIDE)
        cmds.refresh(f=1)

        activeRenderers = cmds.mtoh(listActiveRenderers=1)
        self.assertEqual(activeRenderers, cmds.mtoh(lar=1))
        self.assertEqual(activeRenderers, [mtohUtils.HD_STORM])

        cmds.modelEditor(
            activeEditor, e=1,
            rendererOverrideName="mtohRenderOverride_HdEmbreeRendererPlugin")
        cmds.refresh(f=1)

        activeRenderers = cmds.mtoh(listActiveRenderers=1)
        self.assertEqual(activeRenderers, cmds.mtoh(lar=1))
        self.assertEqual(activeRenderers, ["HdEmbreeRendererPlugin"])

        cmds.modelEditor(activeEditor, rendererOverrideName="", e=1)
        cmds.refresh(f=1)

        activeRenderers = cmds.mtoh(listActiveRenderers=1)
        self.assertEqual(activeRenderers, cmds.mtoh(lar=1))
        self.assertEqual(activeRenderers, [])

    def test_getRendererDisplayName(self):
        # needs at least one arg
        self.assertRaises(RuntimeError, mel.eval,
                          "moth -getRendererDisplayName")

        displayName = cmds.mtoh(renderer=mtohUtils.HD_STORM,
                                getRendererDisplayName=True)
        self.assertEqual(displayName, cmds.mtoh(r=mtohUtils.HD_STORM, gn=True))
        self.assertEqual(displayName, "GL")

        displayName = cmds.mtoh(renderer="HdEmbreeRendererPlugin",
                                getRendererDisplayName=True)
        self.assertEqual(displayName, cmds.mtoh(r="HdEmbreeRendererPlugin",
                                                gn=True))
        self.assertEqual(displayName, "Embree")

    def test_listDelegates(self):
        delegates = cmds.mtoh(listDelegates=1)
        self.assertEqual(delegates, cmds.mtoh(ld=1))
        self.assertIn("HdMayaSceneDelegate", delegates)

    def test_createRenderGlobals(self):
        for flag in ("createRenderGlobals", "crg"):
            cmds.file(f=1, new=1)
            self.assertFalse(cmds.objExists(
                "defaultRenderGlobals.mtohEnableMotionSamples"))
            cmds.mtoh(**{flag: 1})
            self.assertTrue(cmds.objExists(
                "defaultRenderGlobals.mtohEnableMotionSamples"))
            self.assertFalse(cmds.getAttr(
                "defaultRenderGlobals.mtohEnableMotionSamples"))

    # TODO: test_updateRenderGlobals


if __name__ == "__main__":
    testCases = [TestCommand]
    testSuites = []
    for testCase in testCases:
        testSuites.append(unittest.TestLoader().loadTestsFromTestCase(testCase))
    suite = unittest.TestSuite(testSuites)

    results = unittest.TextTestRunner(stream=sys.__stderr__).run(suite)
    if results.wasSuccessful():
        exitCode = 0
    else:
        exitCode = 1

    # maya running interactively often absorbs all the output.  comment out the
    # following to prevent maya from exiting and open the script editor to look
    # at failures.
    cmds.quit(abort=True, exitCode=exitCode)
